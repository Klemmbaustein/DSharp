#include <ds/jit/justInTimeCompiler_x64.hpp>
#include <array>
#include <ds/jit/justInTime.hpp>
#include <ds/modules/system.async.hpp>

using namespace ds;
using namespace ds::jit;
using namespace asmjit;
using namespace asmjit::x86;

class MyErrorHandler : public ErrorHandler
{
public:
	void handle_error(Error err, const char* message, BaseEmitter* origin) override
	{
		printf("AsmJit error: %s\n", message);
	}
};

static RuntimeFunction jit_unrefPtr(RuntimeClass** ptr)
{
	// C++ can really do whatever it wants when compiling reference values
	// so do this to properly pass the class pointer
	return RuntimeClass::unref(*ptr);
}

static void jit_getStructMember(JustInTimeRuntime* rt, Size size, Size offset, Size structSize)
{
	auto targetPos = rt->stackPos - offset - size;
	rt->stackPos -= structSize - size;
	memcpy(&rt->stack[rt->stackPos - size], &rt->stack[targetPos], size);
}

static void jit_setStructMember(JustInTimeRuntime* rt, Size size, Size offset, Size structSize)
{
	auto targetPos = rt->stackPos - offset - size;
	memcpy(&rt->stack[targetPos], &rt->stack[rt->stackPos - structSize - size], size);
	memmove(&rt->stack[rt->stackPos - structSize - size], &rt->stack[rt->stackPos - structSize], structSize);
}

static bool jit_classIs(JustInTimeRuntime* rt, RuntimeClass* cls, ds::TypeId id)
{
	return id == cls->type || rt->runtime->reflect->isSubclassOf(cls->type, id);
}

static void jit_abort(JustInTimeRuntime* rt, const char* msg)
{
	rt->runtimePanic(msg);
}

static RuntimeClass* jit_classAs(JustInTimeRuntime* rt, RuntimeClass* ptr, ds::TypeId id)
{
	if (ptr && id != ptr->type)
	{
		auto [success, offset] = rt->runtime->reflect->tryCast(ptr->type, id);

		if (success)
		{
			ptr = reinterpret_cast<RuntimeClass*>(ptr->getBody() + offset);
		}
		else
		{
			ptr = nullptr;
		}
	}
	return ptr;
}

static void jit_awaitTask(RuntimeClass* task, RuntimeClass* returnTask, JustInTimeRuntime* rt, void* location)
{
	ClassRef<modules::system::async::Task> taskObj = task;

	// if (rt->canAwait)
	//{
	//	rt->suspendLocation = location;
	//	taskObj->awaiter = rt;
	//	return;
	// }
	auto& newRuntime = rt->runtime->asyncContexts.emplace_back(rt->createSuspendedCopy(location));
	taskObj->awaiter = newRuntime;
	rt->pushValue(returnTask);
}

JustInTimeCode* ds::jit::JustInTimeCompiler::compileBytecode(BinaryBuffer& code,
	const std::vector<ds::ExternalFunctionPointer>& pointers,
	std::vector<ds::RuntimeFunction>& vTable, ReflectInfo& reflect, UnwindInfo& unwind)
{
	result->compiled.init(result->jit.environment(), result->jit.cpu_features());

	MyErrorHandler handler;
	result->compiled.set_error_handler(&handler);

	this->assembler = new asmjit::x86::Assembler(&result->compiled);

	scanForFunctions(code, vTable, reflect, unwind);
	buildProlog();
	compileToAssembly(code, pointers, vTable);
	generateEmbeddedStrings();

	result->jit.add(&result->entry, &result->compiled);
	buildVTable(vTable);
	updateReflectionOffsets(reflect);
	updateUnwindOffsets(unwind);

	return result;
}

ds::jit::JustInTimeCompiler::~JustInTimeCompiler()
{
	delete assembler;
}

void ds::jit::JustInTimeCompiler::scanForFunctions(BinaryBuffer& code, std::vector<ds::RuntimeFunction>& vTable,
	ReflectInfo& reflect, UnwindInfo& unwind)
{
	std::set<BytecodeOffset> functionOffsets;

	std::array<uint8_t, 255> argumentBuffer{};
	while (!code.empty())
	{
		auto op = code.getValue<BytecodeOp>();

		uint8_t argsSize = code.getValue<uint8_t>();

		switch (op)
		{
		case ds::BytecodeOp::pushAddr:
		case BytecodeOp::call: {
			code.get(argumentBuffer.data(), argsSize);
			auto found = functionMappings.find(*(BytecodeOffset*)&argumentBuffer[0]);
			if (found == functionMappings.end())
			{
				functionMappings.insert({ *(BytecodeOffset*)&argumentBuffer[0], assembler->new_label() });
			}
			break;
		}
		case BytecodeOp::jumpIf:
		case BytecodeOp::jumpIfNot:
		case BytecodeOp::jump: {
			code.get(argumentBuffer.data(), argsSize);
			auto found = jumpTargetMappings.find(*(BytecodeOffset*)&argumentBuffer[0]);
			if (found == jumpTargetMappings.end())
			{
				jumpTargetMappings.insert({ *(BytecodeOffset*)&argumentBuffer[0], assembler->new_label() });
			}
			break;
		}
		case ds::BytecodeOp::awaitTask: {
			code.get(argumentBuffer.data(), argsSize);
			auto found = jumpTargetMappings.find(*(BytecodeOffset*)&argumentBuffer[sizeof(Size)]);
			if (found == jumpTargetMappings.end())
			{
				jumpTargetMappings.insert({ *(BytecodeOffset*)&argumentBuffer[sizeof(Size)], assembler->new_label() });
			}
			break;
		}
		default:
			code.streamPos += argsSize;
		}
	}


	for (auto& i : vTable)
	{
		if (bool(i) && !i.nativeFn)
		{
			auto found = functionMappings.find(i.codeOffset);
			if (found == functionMappings.end())
			{
				functionMappings.insert({ BytecodeOffset(i.codeOffset), assembler->new_label() });
			}
		}
	}
	for (auto& i : reflect.types)
	{
		auto found = functionMappings.find(i.second.constructor);
		if (found == functionMappings.end())
		{
			functionMappings.insert({ BytecodeOffset(i.second.constructor), assembler->new_label() });
		}
	}

	for (auto& i : unwind.sections)
	{
		unwindMappings.insert({ BytecodeOffset(i.offset), assembler->new_label() });

		for (auto& j : i.parts)
		{
			if (j.size != 0)
			{
				unwindMappings.insert({ BytecodeOffset(j.size), assembler->new_label() });
			}
			unwindMappings.insert({ BytecodeOffset(j.offset), assembler->new_label() });
		}
	}
}

void ds::jit::JustInTimeCompiler::compileToAssembly(BinaryBuffer& code,
	const std::vector<ds::ExternalFunctionPointer>& pointers,
	std::vector<ds::RuntimeFunction>& vTable)
{
	std::array<uint8_t, 255> argumentBuffer{};

	code.streamPos = 0;

	auto tempStack = ptr_64(rsp, 24);

	while (!code.empty())
	{
		auto foundFunction = functionMappings.find(code.streamPos);

		if (code.streamPos == 0 || foundFunction != functionMappings.end())
		{
			if (foundFunction != functionMappings.end())
			{
				assembler->bind(foundFunction->second);
			}
			assembler->push(rbp);
			assembler->mov(rbp, rsp);
			assembler->sub(rsp, 64);
			assembler->mov(runtime, runtimeRegister);
		}

		auto foundJump = jumpTargetMappings.find(code.streamPos);

		if (foundJump != jumpTargetMappings.end())
		{
			flushStack();
			assembler->bind(foundJump->second);
		}

		auto foundUnwind = unwindMappings.find(code.streamPos);

		if (foundUnwind != unwindMappings.end())
		{
			assembler->bind(foundUnwind->second);
		}

		auto op = code.getValue<BytecodeOp>();

		uint8_t argsSize = code.getValue<uint8_t>();

		if (argsSize)
		{
			code.get(argumentBuffer.data(), argsSize);
		}

		switch (op)
		{
		case ds::BytecodeOp::pushAddr:

			flushStack();
			assembler->lea(rax, ptr_64(functionMappings.at(*(int64_t*)&argumentBuffer[0])));
			compilePushValue(rax);
			break;
		case ds::BytecodeOp::push:

			switch (argsSize)
			{
			case 0:
				break;
			case 1:
			case 4:
				compilePushValue(*(int64_t*)&argumentBuffer[0], argsSize);
				break;
			default: {
				flushStack();
				auto l = assembler->new_label();

				std::vector<uint8_t> args = { argumentBuffer.begin(), argumentBuffer.end() };
				args.resize(argsSize);

				auto& result = embeddedStrings.emplace_back(args, l);

				assembler->mov(rdi, stackRegister);
				assembler->lea(rsi, ptr_64(result.second));
				compileMemoryCopy(argsSize);
				changeStackBy(argsSize);
				break;
			}
			}
			break;
		case ds::BytecodeOp::pop: {
			Size size = *(int64_t*)&argumentBuffer[0];
			changeStackBy(-size);
			break;
		}
		case ds::BytecodeOp::copy: {
			flushStack();
			auto size = *(BytecodeOffset*)&argumentBuffer[0];
			switch (size)
			{
			case 1:
				assembler->mov(al, ptr_8(stackRegister, -1));
				assembler->mov(ptr_8(stackRegister), al);
				break;
			case 4: {
				assembler->mov(eax, ptr_32(stackRegister, -4));
				assembler->mov(ptr_32(stackRegister), eax);
				break;
			}
			case 8:
				assembler->mov(rax, ptr_64(stackRegister, -8));
				assembler->mov(ptr_64(stackRegister), rax);
				break;
			default:
				abort();
			}
			changeStackBy(size);
			break;
		}
		case ds::BytecodeOp::call: {

			flushStack();
			assembler->mov(ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, lastStackPos)), rbp);
			assembler->call(functionMappings.at(*(BytecodeOffset*)&argumentBuffer[0]));
			break;
		}
		case ds::BytecodeOp::jump: {
			flushStack();
			assembler->jmp(jumpTargetMappings.at(*(BytecodeOffset*)&argumentBuffer[0]));
			break;
		}
		case ds::BytecodeOp::jumpIfNot: {
			auto val = compilePopValue(sizeof(Bool), true);
			if (!val.isNumber)
			{
				assembler->test(val.gpRegister, val.gpRegister);

				assembler->jz(jumpTargetMappings.at(*(BytecodeOffset*)&argumentBuffer[0]));
			}
			else if (!uint8_t(val.number))
			{
				assembler->jmp(jumpTargetMappings.at(*(BytecodeOffset*)&argumentBuffer[0]));
			}
			break;
		}
		case ds::BytecodeOp::jumpIf: {
			auto val = compilePopValue(sizeof(Bool), true);
			if (!val.isNumber)
			{
				assembler->test(val.gpRegister, val.gpRegister);

				assembler->jnz(jumpTargetMappings.at(*(BytecodeOffset*)&argumentBuffer[0]));
			}
			else if (uint8_t(val.number))
			{
				assembler->jmp(jumpTargetMappings.at(*(BytecodeOffset*)&argumentBuffer[0]));
			}
			break;
		}
		case ds::BytecodeOp::boolNot: {
			auto val = compilePopValue(1, true);
			if (val.isNumber)
			{
				bool valueBool = bool(val.number);
				compilePushValue(!valueBool, 1);
			}
			else
			{
				assembler->test(val.gpRegister, val.gpRegister);
				assembler->setz(val.gpRegister);
				compilePushValue(val.gpRegister);
			}
			break;
		}
		case ds::BytecodeOp::callExternal: {
			flushStack();
			assembler->mov(ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, lastStackPos)), rbp);
			assembler->mov(argumentRegisters[0], runtimeRegister);
			assembler->call(pointers.at(*(BytecodeOffset*)&argumentBuffer[0]));
			restoreRegisters();
			break;
		}
		case ds::BytecodeOp::addInt: {
			// Upper stack value -> eax
			auto result = compilePopValue(sizeof(Int), false);
			// Lower stack value += eax

			int32_t stackValue = result.stackDiff + sizeof(Int);
			if (result.isNumber)
			{
				assembler->add(ptr_32(stackRegister, -stackValue), result.number);
			}
			else
			{
				assembler->add(ptr_32(stackRegister, -stackValue), result.gpRegister);
			}
			changeStackBy(-(sizeof(Int) + result.stackDiff) + sizeof(Int));
			break;
		}
		case ds::BytecodeOp::subInt: {
			// Upper stack value -> eax
			auto result = compilePopValue(sizeof(Int), false);
			// Lower stack value += eax

			int32_t stackValue = result.stackDiff + sizeof(Int);
			if (result.isNumber)
			{
				assembler->sub(ptr_32(stackRegister, -stackValue), result.number);
			}
			else
			{
				assembler->sub(ptr_32(stackRegister, -stackValue), result.gpRegister);
			}
			changeStackBy(-(sizeof(Int) + result.stackDiff) + sizeof(Int));
			break;
		}
		case ds::BytecodeOp::mulInt:
			flushStack();
			// Upper stack value -> eax
			assembler->mov(eax, ptr_32(stackRegister, -4));
			// Lower stack value -> r8d
			assembler->mov(r8d, ptr_32(stackRegister, -8));
			assembler->imul(eax, r8d);
			// Move the result back into the stack
			assembler->mov(ptr_32(stackRegister, -8), eax);
			changeStackBy(-4);
			break;
		case ds::BytecodeOp::divInt:
			flushStack();
			// Upper stack value -> eax
			assembler->mov(eax, ptr_32(stackRegister, -8));
			// Lower stack value -> r8d
			assembler->mov(r8d, ptr_32(stackRegister, -4));
			assembler->mov(rdx, 0);
			assembler->idiv(r8d);
			assembler->mov(runtimeRegister, runtime);
			// Move the result back into the stack
			assembler->mov(ptr_32(stackRegister, -8), eax);
			changeStackBy(-4);
			break;
		case ds::BytecodeOp::modInt:
			flushStack();
			// Upper stack value -> eax
			assembler->mov(eax, ptr_32(stackRegister, -8));
			// Lower stack value -> r8d
			assembler->mov(r8d, ptr_32(stackRegister, -4));
			assembler->mov(rdx, 0);
			assembler->idiv(r8d);
			// Move the result back into the stack
			assembler->mov(ptr_32(stackRegister, -8), edx);
			assembler->mov(runtimeRegister, runtime);
			changeStackBy(-4);
			break;
		case ds::BytecodeOp::negativeInt: {
			auto val = compilePopValue(sizeof(Int), true);

			if (val.isNumber)
			{
				compilePushValue(-Int(val.number), sizeof(Int));
			}
			else
			{
				assembler->neg(val.gpRegister);
				compilePushValue(val.gpRegister);
			}
			break;
		}
		case ds::BytecodeOp::addFloat:
			flushStack();
			// Lower stack value -> float register
			assembler->fld(ptr_32(stackRegister, -8));
			// Add higher stack value to it
			assembler->fadd(ptr_32(stackRegister, -4));
			// Move the result back into the stack
			assembler->fstp(ptr_32(stackRegister, -8));
			changeStackBy(-4);
			break;
		case ds::BytecodeOp::subFloat:
			flushStack();
			// Lower stack value -> float register
			assembler->fld(ptr_32(stackRegister, -8));
			// Add higher stack value to it
			assembler->fsub(ptr_32(stackRegister, -4));
			// Move the result back into the stack
			assembler->fstp(ptr_32(stackRegister, -8));
			changeStackBy(-4);
			break;
		case ds::BytecodeOp::mulFloat:
			flushStack();
			// Lower stack value -> float register
			assembler->fld(ptr_32(stackRegister, -8));
			// Add multiply stack value to it
			assembler->fmul(ptr_32(stackRegister, -4));
			// Move the result back into the stack
			assembler->fstp(ptr_32(stackRegister, -8));
			changeStackBy(-4);
			break;
		case ds::BytecodeOp::divFloat:
			flushStack();
			// Lower stack value -> float register
			assembler->fld(ptr_32(stackRegister, -8));
			// Add divide stack value with it
			assembler->fdiv(ptr_32(stackRegister, -4));
			// Move the result back into the stack
			assembler->fstp(ptr_32(stackRegister, -8));
			changeStackBy(-4);
			break;
		case ds::BytecodeOp::negativeFloat:
			flushStack();
			// Lower stack value -> float register
			assembler->fldz();
			assembler->fsub(ptr_32(stackRegister, -4));
			// Move the result back into the stack
			assembler->fstp(ptr_32(stackRegister, -4));
			break;
		case ds::BytecodeOp::floatToInt: {
			flushStack();
			assembler->fld(ptr_32(stackRegister, -4));
			assembler->fisttp(ptr_32(stackRegister, -4));
			break;
		}
		case ds::BytecodeOp::intToFloat: {
			flushStack();
			assembler->fild(ptr_32(stackRegister, -4));
			assembler->fstp(ptr_32(stackRegister, -4));
			break;
		}
		case ds::BytecodeOp::greaterInt: {
			auto argument = compilePopValue(4, false);
			int32_t stackPosition = -(sizeof(Int) + argument.stackDiff);
			if (argument.isNumber)
			{
				assembler->cmp(ptr_32(stackRegister, stackPosition), argument.number);
			}
			else
			{
				assembler->cmp(ptr_32(stackRegister, stackPosition), argument.gpRegister);
			}
			auto successLabel = assembler->new_label();
			auto failLabel = assembler->new_label();

			assembler->setg(al);
			changeStackBy(-(sizeof(Int) + argument.stackDiff));
			compilePushValue(al);
			break;
		}
		case ds::BytecodeOp::greaterFloat: {
			flushStack();
			// Lower stack value -> float register
			assembler->movd(xmm0, ptr_32(stackRegister, -8));
			assembler->movd(xmm1, ptr_32(stackRegister, -4));
			// Move the result back into the stack
			assembler->ucomiss(xmm0, xmm1);
			assembler->seta(al);
			changeStackBy(-sizeof(Float) * 2);
			compilePushValue(al);
			break;
		}
		case ds::BytecodeOp::equals: {
			flushStack();
			auto size = *(Size*)&argumentBuffer[0];

			bool isStandardSize = true;
			switch (size)
			{
			case 1:
				assembler->mov(al, ptr_8(stackRegister, -1));
				assembler->cmp(ptr_8(stackRegister, -2), al);
				break;
			case 4: {
				assembler->mov(eax, ptr_32(stackRegister, -4));
				assembler->cmp(ptr_32(stackRegister, -8), eax);
				break;
			}
			case 8:
				assembler->mov(rax, ptr_64(stackRegister, -8));
				assembler->cmp(ptr_64(stackRegister, -16), rax);
				break;
			default:
				isStandardSize = false;
				abort();
			}

			if (isStandardSize)
			{
				assembler->sete(ptr_8(stackRegister, size * -2));
			}

			changeStackBy(size * -2 + sizeof(Bool));
			break;
		}
		case ds::BytecodeOp::ret:
			flushStack();
			assembler->leave();
			assembler->ret();
			break;
		case ds::BytecodeOp::storeVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];

			auto popped = compilePopValue(size, true);

			if (popped.isNumber)
			{
				switch (size)
				{
				case 1:
					assembler->mov(ptr_8(r9, -offset), popped.number);
					break;
				case 4: {
					assembler->mov(ptr_32(r9, -offset), popped.number);
					break;
				}
				case 8:
					assembler->mov(ptr_64(r9, -offset), popped.number);
					break;
				default:
					abort();
				}
			}
			else
			{
				switch (size)
				{
				case 1:
					assembler->mov(ptr_8(r9, -offset), popped.gpRegister);
					break;
				case 4: {
					assembler->mov(ptr_32(r9, -offset), popped.gpRegister);
					break;
				}
				case 8:
					assembler->mov(ptr_64(r9, -offset), popped.gpRegister);
					break;
				default:
					assembler->lea(rdi, ptr_64(r9, -offset));
					assembler->mov(rsi, popped.gpRegister);
					compileMemoryCopy(size);
					break;
				}
			}
			break;
		}
		case ds::BytecodeOp::readVariable: {
			flushStack();
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];

			Gp target;
			bool isDefaultSize = false;

			switch (size)
			{
			case 1:
				assembler->mov(al, ptr_8(r9, -offset));
				target = al;
				isDefaultSize = true;
				break;
			case 4: {
				assembler->mov(eax, ptr_32(r9, -offset));
				target = eax;
				isDefaultSize = true;
				break;
			}
			case 8:
				assembler->mov(rax, ptr_64(r9, -offset));
				target = rax;
				isDefaultSize = true;
				break;
			default:
				assembler->mov(rdi, stackRegister);
				assembler->lea(rsi, ptr_64(r9, -offset));
				compileMemoryCopy(size);
				changeStackBy(size);
				break;
			}

			if (isDefaultSize)
			{
				compilePushValue(target);
			}
			break;
		}
		case ds::BytecodeOp::pushVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			assembler->add(varStackPos, size);
			assembler->add(r9, size);
			break;
		}
		case ds::BytecodeOp::popVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			assembler->sub(varStackPos, size);
			assembler->sub(r9, size);
			break;
		}
		case ds::BytecodeOp::allocClass: {
			int32_t size = compilePopValueToRegister(halfArgumentRegisters[0], false);
			Size typeId = *(Size*)&argumentBuffer[0];
			BytecodeOffset vTableOffset = *(BytecodeOffset*)&argumentBuffer[sizeof(typeId)];
			assembler->mov(halfArgumentRegisters[1], typeId);
			auto offsetPtr = vTableOffset != UINT32_MAX ? (&vTable[vTableOffset]) : nullptr;
			assembler->mov(argumentRegisters[2], offsetPtr);
			assembler->call(RuntimeClass::allocateClass);
			restoreRegisters();
			assembler->mov(ptr_64(stackRegister, -size), rax);
			changeStackBy(sizeof(Pointer) - size);
			break;
		}
		case ds::BytecodeOp::classMemberPtr:
		case ds::BytecodeOp::classMember: {
			flushStack();

			bool isPtr = op == ds::BytecodeOp::classMemberPtr;

			assembler->xor_(rax, rax);
			// Offset
			assembler->mov(eax, ptr_32(stackRegister, -8));
			// class body ptr
			assembler->mov(rsi, ptr_64(stackRegister, -16));
			if (isPtr)
			{
				assembler->mov(rsi, ptr_64(rsi, sizeof(RuntimeClass)));
			}
			else
			{
				assembler->add(rsi, sizeof(RuntimeClass));
			}
			assembler->add(rsi, rax);
			// Size (overwrites offset)
			assembler->mov(eax, ptr_32(stackRegister, -4));
			assembler->mov(r10, rax);
			// Destination on the stack
			assembler->lea(rdi, ptr_64(stackRegister, -16));
			changeStackBy(-16);

			compileMemoryCopy(eax);

			assembler->add(stackPos, r10);
			assembler->add(stackRegister, r10);
			break;
		}
		case ds::BytecodeOp::setClassMember:
		case ds::BytecodeOp::setClassMemberPtr:
		case ds::BytecodeOp::setClassMemberPushAgain: {

			flushStack();

			bool pushAgain = op == ds::BytecodeOp::setClassMemberPushAgain;
			bool isPtr = op == ds::BytecodeOp::setClassMemberPtr;

			assembler->xor_(rax, rax);
			// Offset
			assembler->mov(eax, ptr_32(stackRegister, -8));
			// class body ptr
			assembler->mov(rdi, ptr_64(stackRegister, -16));
			if (pushAgain)
			{
				assembler->mov(r10, rdi);
			}
			if (isPtr)
			{
				assembler->mov(rdi, ptr_64(rdi, sizeof(RuntimeClass)));
			}
			else
			{
				assembler->add(rdi, sizeof(RuntimeClass));
			}
			assembler->add(rdi, rax);
			// Size (overwrites offset)
			assembler->mov(eax, ptr_32(stackRegister, -4));
			// Data to write to the class
			assembler->lea(rsi, ptr_64(stackRegister, -16));
			assembler->sub(rsi, rax);
			assembler->add(eax, 16);
			assembler->sub(ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, stackPos)), rax);
			assembler->sub(stackRegister, rax);
			assembler->sub(eax, 16);

			compileMemoryCopy(eax);

			if (pushAgain)
			{
				compilePushValue(r10);
			}
			break;
		}
		case ds::BytecodeOp::refClass: {
			compilePopValueToRegister(rax, true);
			auto nullLabel = assembler->new_label();

			assembler->test(rax, rax);
			assembler->jz(nullLabel);

			auto ref = [](RuntimeClass* target) {
				target->addRef();
			};
			assembler->mov(tempStack, rax);
			assembler->mov(argumentRegisters[0], rax);
			assembler->call((void (*)(RuntimeClass*))ref);
			restoreRegisters();
			assembler->mov(rax, tempStack);
			assembler->bind(nullLabel);
			compilePushValue(rax);
			break;
		}
		case ds::BytecodeOp::unrefClass: {
			flushStack();
			auto nullLabel = assembler->new_label();
			assembler->mov(rax, ptr_64(stackRegister, -8));
			assembler->mov(ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, lastStackPos)), rbp);

			assembler->test(rax, rax);
			assembler->jz(nullLabel);
			assembler->mov(tempStack, rax);
#ifdef _WIN32
			assembler->lea(argumentRegisters[1], tempStack);
#else
			assembler->lea(argumentRegisters[0], tempStack);
#endif
			assembler->call(jit_unrefPtr);
			assembler->mov(argumentRegisters[0], returnValueRegister);

			auto testFunction = [](ds::RuntimeFunction f) {
				bool b = bool(f);

				return b;
			};

#ifdef __linux__
			assembler->mov(argumentRegisters[1], rdx);
#endif

			assembler->call((bool (*)(ds::RuntimeFunction))testFunction);

			assembler->test(al, al);

			auto endLabel = assembler->new_label();
			auto noPopLabel = assembler->new_label();

			assembler->jz(endLabel);
			assembler->mov(runtimeRegister, runtime);
			getStack();

			assembler->mov(rax, tempStack);
			assembler->mov(ptr_64(stackRegister, -8), rax);
			assembler->mov(rax, ptr_64(rax, DS_OFFSETOF(RuntimeClass, vtable)));
			assembler->mov(r8, ptr_64(rax, DS_OFFSETOF(RuntimeFunction, codeOffset)));
			// Check if it's a script function
			assembler->cmp(r8, UINTPTR_MAX);

			auto nativeFunctionLabel = assembler->new_label();

			assembler->je(nativeFunctionLabel);
			restoreRegisters();
			assembler->call(r8);
			assembler->jmp(noPopLabel);
			assembler->bind(nativeFunctionLabel);
			assembler->mov(argumentRegisters[0], runtime);
			assembler->call(ptr_64(rax, DS_OFFSETOF(RuntimeFunction, nativeFn)));
			restoreRegisters();
			assembler->jmp(noPopLabel);

			assembler->bind(endLabel);

			restoreRegisters();

			assembler->bind(nullLabel);

			changeStackBy(-8);
			assembler->bind(noPopLabel);
			break;
		}
		case ds::BytecodeOp::virtualCall: {
			flushStack();
			BytecodeOffset called = *(BytecodeOffset*)&argumentBuffer[0];
			assembler->mov(rax, ptr_64(stackRegister, -sizeof(Pointer)));

			assembler->mov(rax, ptr_64(rax, DS_OFFSETOF(RuntimeClass, vtable)));
			assembler->mov(r8, ptr_64(rax, DS_OFFSETOF(RuntimeFunction, codeOffset) + sizeof(RuntimeFunction) * called));
			// Check if it's a script function
			assembler->cmp(r8, UINTPTR_MAX);

			auto nativeFunctionLabel = assembler->new_label();
			auto endLabel = assembler->new_label();
			assembler->je(nativeFunctionLabel);

			assembler->mov(ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, lastStackPos)), rbp);
			assembler->call(r8);
			assembler->jmp(endLabel);

			assembler->bind(nativeFunctionLabel);
			assembler->mov(r8, ptr_64(rax, DS_OFFSETOF(RuntimeFunction, nativeFn) + sizeof(RuntimeFunction) * called));
			assembler->test(r8, r8);
			assembler->jz(endLabel);
			assembler->mov(ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, lastStackPos)), rbp);
			assembler->mov(argumentRegisters[0], runtimeRegister);
			assembler->call(r8);
			restoreRegisters();

			assembler->bind(endLabel);
			break;
		}
		case ds::BytecodeOp::castInterface: {
			compilePopValueToRegister(rax, true);

			Int offset = *(Int*)&argumentBuffer[0];
			Bool unCast = *(Bool*)&argumentBuffer[sizeof(offset)];

			if (unCast)
			{
				assembler->sub(rax, offset + sizeof(RuntimeClass));
			}
			else
			{
				assembler->add(rax, offset + sizeof(RuntimeClass));
			}

			compilePushValue(rax);

			break;
		}
		case ds::BytecodeOp::implInterface: {

			compilePopValueToRegister(rax, true);
			BytecodeOffset offset = *(BytecodeOffset*)&argumentBuffer[0];
			BytecodeOffset offsetBytes = offset + sizeof(RuntimeClass);
			BytecodeOffset vTableOffset = *(BytecodeOffset*)&argumentBuffer[sizeof(offset)];

			auto offsetPtr = vTableOffset != UINT32_MAX ? (&vTable[vTableOffset]) : nullptr;
			assembler->mov(r8, Pointer(offsetPtr));
			assembler->mov(ptr_64(rax, DS_OFFSETOF(RuntimeClass, vtable) + offsetBytes), r8);
			assembler->mov(ptr_32(rax, DS_OFFSETOF(RuntimeClass, references) + offsetBytes), offset);
			assembler->mov(ptr_8(rax, DS_OFFSETOF(RuntimeClass, referencesAreOffset) + offsetBytes), 1);

			assembler->mov(r8d, ptr_32(rax, DS_OFFSETOF(RuntimeClass, type)));
			assembler->mov(ptr_32(rax, DS_OFFSETOF(RuntimeClass, type) + offsetBytes), r8d);

			compilePushValue(rax);

			break;
		}
		case ds::BytecodeOp::getStructMember: {
			flushStack();
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			Size structSize = *(Size*)&argumentBuffer[sizeof(size) + sizeof(offset)];

			switch (size)
			{
			case 1:
				changeStackBy(-structSize);
				assembler->mov(al, ptr_8(stackRegister, structSize - offset - size));
				compilePushValue(al);
				break;
			case 4:
				changeStackBy(-structSize);
				assembler->mov(eax, ptr_32(stackRegister, structSize - offset - size));
				compilePushValue(eax);
				break;
			case 8:
				changeStackBy(-structSize);
				assembler->mov(rax, ptr_64(stackRegister, structSize - offset - size));
				compilePushValue(rax);
				break;
			default:
				flushStack();
				assembler->mov(argumentRegisters[0], runtimeRegister);
				assembler->mov(halfArgumentRegisters[1], size);
				assembler->mov(halfArgumentRegisters[2], offset);
				assembler->mov(halfArgumentRegisters[3], structSize);

				assembler->call(jit_getStructMember);
				restoreRegisters();
				break;
			}
			break;
		}
		case ds::BytecodeOp::classIs: {
			compilePopValueToRegister(r8, true);

			TypeId id = *(TypeId*)&argumentBuffer[0];

			auto nullLabel = assembler->new_label();

			assembler->test(r8, r8);
			assembler->setnz(al);
			assembler->jz(nullLabel);

			assembler->mov(argumentRegisters[0], runtimeRegister);
			assembler->mov(argumentRegisters[1], r8);
			assembler->mov(argumentRegisters[2], id);
			assembler->call(jit_classIs);
			restoreRegisters();

			assembler->bind(nullLabel);
			compilePushValue(al);

			break;
		}
		case ds::BytecodeOp::classAs: {
			TypeId id = *(TypeId*)&argumentBuffer[0];
			Bool isNullable = *(Bool*)&argumentBuffer[sizeof(TypeId)];
			compilePopValueToRegister(rax, true);

			assembler->mov(argumentRegisters[0], runtimeRegister);
			assembler->mov(argumentRegisters[1], rax);
			assembler->mov(argumentRegisters[2], id);
			assembler->call(jit_classAs);
			restoreRegisters();

			if (!isNullable)
			{
				assembler->test(rax, rax);

				auto notNullLabel = assembler->new_label();

				assembler->jnz(notNullLabel);
				// TODO: replace with unwinding
				compileAbort("Non nullable cast failed.");
				assembler->bind(notNullLabel);
			}

			compilePushValue(rax);
			break;
		}
		case ds::BytecodeOp::setStructMember: {
			flushStack();
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			Size structSize = *(Size*)&argumentBuffer[sizeof(size) + sizeof(offset)];
			assembler->mov(argumentRegisters[0], runtimeRegister);
			assembler->mov(halfArgumentRegisters[1], size);
			assembler->mov(halfArgumentRegisters[2], offset);
			assembler->mov(halfArgumentRegisters[3], structSize);
			assembler->call(jit_setStructMember);
			restoreRegisters();

			changeStackBy(-size);
			break;
		}
		case ds::BytecodeOp::nullCheck: {
			compilePopValueToRegister(rax, true);

			assembler->test(rax, rax);
			auto notNullLabel = assembler->new_label();

			assembler->jnz(notNullLabel);

			compileAbort("Attempted to use null reference");

			assembler->bind(notNullLabel);

			compilePushValue(rax);
			break;
		}
		case ds::BytecodeOp::awaitTask: {
			Size resultSize = *(Size*)&argumentBuffer[0];
			BytecodeOffset newPos = *(BytecodeOffset*)&argumentBuffer[sizeof(resultSize)];

			auto& foundMapping = jumpTargetMappings.at(newPos);

			compilePopValueToRegister(rax, true);
			changeStackBy(-8);
			assembler->mov(r8b, ptr_8(rax, DS_OFFSETOF(modules::system::async::Task, completed) + sizeof(RuntimeClass)));
			assembler->test(r8b, r8b);
			assembler->jnz(foundMapping);

			assembler->mov(argumentRegisters[1], ptr_64(stackRegister));
			assembler->mov(argumentRegisters[0], rax);
			assembler->mov(argumentRegisters[2], runtime);
			assembler->lea(argumentRegisters[3], ptr_64(foundMapping));

			assembler->call(jit_awaitTask);
			restoreRegisters();


			break;
		}
		case ds::BytecodeOp::noReturn: {
			compileAbort("Function did not return");
			break;
		}
		default:
			abort();
		}
	}
	auto foundUnwind = unwindMappings.find(code.streamPos);

	if (foundUnwind != unwindMappings.end())
	{
		assembler->bind(foundUnwind->second);
	}
}

void ds::jit::JustInTimeCompiler::buildVTable(std::vector<ds::RuntimeFunction>& vTable)
{
	for (auto& i : vTable)
	{
		if (bool(i) && !i.nativeFn)
		{
			auto found = functionMappings.find(i.codeOffset);

			if (found != functionMappings.end() && result->compiled.is_label_bound(found->second))
			{
				i.codeOffset = Pointer(result->entry) + result->compiled.label_offset(found->second);
			}
			else
			{
				i.codeOffset = UINTPTR_MAX;
			}
		}
	}
}

void ds::jit::JustInTimeCompiler::updateReflectionOffsets(ReflectInfo& reflect)
{
	for (auto& i : reflect.types)
	{
		i.second.constructor = Pointer(result->entry) + result->compiled.label_offset(functionMappings.at(i.second.constructor));
	}
}

void ds::jit::JustInTimeCompiler::updateUnwindOffsets(UnwindInfo& unwind)
{
	for (auto& i : unwind.sections)
	{
		i.offset = Pointer(result->entry) + result->compiled.label_offset(unwindMappings.at(i.offset));

		for (auto& j : i.parts)
		{
			if (j.start != 0)
			{
				j.start = Pointer(result->entry) + result->compiled.label_offset(unwindMappings.at(j.start));
			}
			j.offset = Pointer(result->entry) + result->compiled.label_offset(unwindMappings.at(j.offset));
		}
	}
}

void ds::jit::JustInTimeCompiler::buildProlog()
{
	// Function prolog.
	assembler->push(rbp);
	assembler->mov(rbp, rsp);
	assembler->sub(rsp, 64 + 16);
	assembler->mov(ptr_64(rsp, 64), MANAGED_STACK_BEGIN_MARKER);
#if _WIN32 // The windows x64 calling convention makes rsi and rdi nonvolatile, meaning they have to be saved.
	assembler->push(rsi);
	assembler->push(rdi);
#endif

	assembler->mov(rax, argumentRegisters[0]);
	assembler->mov(runtime, argumentRegisters[1]);

	restoreRegisters();
	assembler->call(rax);
#if _WIN32
	assembler->pop(rdi);
	assembler->pop(rsi);
#endif
	assembler->leave();
	assembler->ret();

	Label resumeLabel = assembler->new_named_label("resume");
	assembler->bind(resumeLabel);
	// Async resume prolog.
	assembler->push(rbp);
	assembler->mov(rbp, rsp);
	assembler->sub(rsp, 64 + 16);
	assembler->mov(ptr_64(rsp, 64), MANAGED_STACK_BEGIN_MARKER);
#if _WIN32
	assembler->push(rsi);
	assembler->push(rdi);
#endif

	assembler->mov(rax, argumentRegisters[0]);
	assembler->mov(runtime, argumentRegisters[1]);

	restoreRegisters();

	auto resumeProcLabel = assembler->new_label();

	assembler->call(resumeProcLabel);

#if _WIN32
	assembler->pop(rdi);
	assembler->pop(rsi);
#endif
	assembler->leave();
	assembler->ret();

	assembler->bind(resumeProcLabel);
	assembler->push(rbp);
	assembler->mov(rbp, rsp);
	assembler->sub(rsp, 64);
	assembler->mov(runtime, runtimeRegister);
	assembler->jmp(rax);
	assembler->leave();
	assembler->ret();

	Label entry = assembler->new_named_label("entry");
	assembler->bind(entry);
}

void ds::jit::JustInTimeCompiler::compilePushValue(asmjit::x86::Gp gpRegister)
{
	if (currentStackValue)
	{
		flushStack();
	}
	currentStackValue = StackValue{ .gpRegister = gpRegister };
}

void ds::jit::JustInTimeCompiler::compilePushValue(size_t value, size_t size)
{
	if (currentStackValue)
	{
		flushStack();
	}
	currentStackValue = StackValue{ .isNumber = true, .number = value, .size = size };
}

void ds::jit::JustInTimeCompiler::compileMemoryCopy(size_t size)
{
	assembler->mov(rax, size);
	compileMemoryCopy(rax);
}

void ds::jit::JustInTimeCompiler::compileMemoryCopy(asmjit::x86::Gp size)
{
	auto loop = assembler->new_label();
	auto end = assembler->new_label();
	assembler->bind(loop);
	assembler->cmp(size, 0);

	assembler->je(end);

	assembler->movsb();
	assembler->dec(size);

	assembler->jmp(loop);
	assembler->bind(end);
}

void ds::jit::JustInTimeCompiler::restoreRegisters()
{
	assembler->mov(runtimeRegister, runtime);
	getStack();

	// Restore variable stack
	assembler->mov(r9, varStackPos);
	assembler->lea(r9, ptr_64(runtimeRegister, r9, 0, DS_OFFSETOF(JustInTimeRuntime, variableStack)));
}

void ds::jit::JustInTimeCompiler::getStack()
{
	// Restore stack
	assembler->mov(stackRegister, stackPos);
	assembler->lea(stackRegister, ptr_64(runtimeRegister, stackRegister, 0, DS_OFFSETOF(JustInTimeRuntime, stack)));
}

void ds::jit::JustInTimeCompiler::compileAbort(const char* msg)
{
	assembler->mov(ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, lastStackPos)), rbp);
	assembler->mov(argumentRegisters[0], runtimeRegister);
	assembler->mov(argumentRegisters[1], msg);
	assembler->call(jit_abort);
}

StackValue ds::jit::JustInTimeCompiler::compilePopValue(size_t size, bool applyStackPos)
{
	if (currentStackValue)
	{
		if (currentStackValue->isNumber && currentStackValue->size != size)
		{
			abort();
		}
		else if (!currentStackValue->isNumber && currentStackValue->gpRegister.size() != size)
		{
			int s = currentStackValue->gpRegister.size();
			throw s;
		}

		StackValue result = *currentStackValue;

		currentStackValue = {};

		return result;
	}

	Gp result;

	switch (size)
	{
	case 1:
		result = al;
		assembler->mov(result, ptr_8(stackRegister, -size));
		break;
	case 4: {
		result = eax;
		assembler->mov(result, ptr_32(stackRegister, -size));
		break;
	}
	case 8:
		result = rax;
		assembler->mov(result, ptr_64(stackRegister, -size));
		break;
	default:
		result = rax;
		assembler->lea(result, ptr_64(stackRegister, -size));
		break;
	}
	if (applyStackPos)
	{
		changeStackBy(-size);
	}
	return StackValue{ .gpRegister = result, .stackDiff = size };
}

int32_t ds::jit::JustInTimeCompiler::compilePopValueToRegister(asmjit::x86::Gp target, bool applyStackPos)
{
	if (currentStackValue)
	{
		if (currentStackValue->isNumber)
		{
			if (currentStackValue->size != target.size())
			{
				abort();
			}
			assembler->mov(target, currentStackValue->number);
		}
		else
		{
			if (currentStackValue->gpRegister.size() != target.size())
			{
				abort();
			}
			if (currentStackValue->gpRegister != target)
			{
				assembler->mov(target, currentStackValue->gpRegister);
			}
		}

		currentStackValue = {};
		return 0;
	}

	auto size = target.size();

	switch (target.size())
	{
	case 1:
		assembler->mov(target, ptr_8(stackRegister, -size));
		break;
	case 4: {
		assembler->mov(target, ptr_32(stackRegister, -size));
		break;
	}
	case 8:
		assembler->mov(target, ptr_64(stackRegister, -size));
		break;
	default:
		abort();
	}

	if (applyStackPos)
	{
		changeStackBy(-target.size());
	}
	return size;
}

void ds::jit::JustInTimeCompiler::changeStackBy(int32_t amount)
{
	if (amount == 1)
	{
		assembler->inc(stackPos);
		assembler->inc(stackRegister);
	}
	else if (amount == -1)
	{
		assembler->dec(stackPos);
		assembler->dec(stackRegister);
	}
	else if (amount > 1)
	{
		assembler->add(stackPos, amount);
		assembler->add(stackRegister, amount);
	}
	else if (amount < -1)
	{
		assembler->sub(stackPos, -amount);
		assembler->sub(stackRegister, -amount);
	}

#if 0 // stack sanity check
	assembler->cmp(stackPos, 0);
	auto okay = assembler->new_label();
	assembler->jge(okay);
	assembler->int3();
	assembler->bind(okay);
#endif
}

void ds::jit::JustInTimeCompiler::flushStack()
{
	if (!currentStackValue)
	{
		return;
	}

	StackValue& val = *currentStackValue;
	currentStackValue = {};

	if (val.isNumber)
	{
		switch (val.size)
		{
		case 1:
			assembler->mov(ptr_8(stackRegister), val.number);
			break;
		case 4: {
			assembler->mov(ptr_32(stackRegister), val.number);
			break;
		}
		case 8:
			assembler->mov(ptr_64(stackRegister), val.number);
			break;
		default:
			abort();
		}
		changeStackBy(val.size);
	}
	else
	{
		switch (val.gpRegister.size())
		{
		case 1:
			assembler->mov(ptr_8(stackRegister), val.gpRegister);
			break;
		case 4: {
			assembler->mov(ptr_32(stackRegister), val.gpRegister);
			break;
		}
		case 8:
			assembler->mov(ptr_64(stackRegister), val.gpRegister);
			break;
		default:
			abort();
		}
		changeStackBy(val.gpRegister.size());
	}
}

void ds::jit::JustInTimeCompiler::generateEmbeddedStrings()
{
	Section* dataSection = nullptr;
	result->compiled.new_section(Out(dataSection), ".data");
	assembler->section(dataSection);

	for (auto& i : this->embeddedStrings)
	{
		assembler->bind(i.second);
		assembler->embed_data_array(asmjit::TypeId::kUInt8, i.first.data(), i.first.size());
	}
}
