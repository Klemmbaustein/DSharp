#include <ds/jit/justInTimeCompiler_x64.hpp>
#include <array>
#include <ds/jit/justInTime.hpp>
#include <ds/modules/system.async.hpp>
#include <cassert>

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
		auto [success, isInterface, offset] = rt->runtime->reflect->tryCast(ptr->type, id);

		if (success)
		{
			ptr = isInterface ? reinterpret_cast<RuntimeClass*>(ptr->getBody() + offset) : ptr;
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

std::shared_ptr<JustInTimeCode> ds::jit::JustInTimeCompiler::compileBytecode(BinaryBuffer& code,
	const std::vector<ds::ExternalFunctionPointer>& pointers,
	std::vector<ds::RuntimeFunction>& vTable, ReflectInfo& reflect, UnwindInfo& unwind,
	DebugInfo* debug)
{
	result->compiled.init(result->jit.environment(), result->jit.cpu_features());

	MyErrorHandler handler;
	result->compiled.set_error_handler(&handler);

	this->assembler = new asmjit::x86::Assembler(&result->compiled);

	scanForFunctions(code, vTable, reflect, unwind, debug);
	buildProlog();
	compileToAssembly(code, pointers, vTable);
	generateEmbeddedStrings();

	result->jit.add(&result->entry, &result->compiled);
	buildVTable(vTable);
	updateReflectionOffsets(reflect);
	updateUnwindOffsets(unwind);

	if (debug)
	{
		updateDebugOffsets(debug);
	}

	return result;
}

ds::jit::JustInTimeCompiler::~JustInTimeCompiler()
{
	delete assembler;
}

void ds::jit::JustInTimeCompiler::scanForFunctions(BinaryBuffer& code, std::vector<ds::RuntimeFunction>& vTable,
	ReflectInfo& reflect, UnwindInfo& unwind, DebugInfo* debug)
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

	if (debug)
	{
		for (auto& i : debug->sections)
		{
			debugMappings.insert({ BytecodeOffset(i.offset), assembler->new_label() });
		}
	}
}

void ds::jit::StackValue::compileTwoOp(StackValue& b, std::function<void()> allNumbers,
	std::function<void()> oneRegister, std::function<void()> allRegisters)
{
	if (this->isNumber)
	{
		if (b.isNumber)
		{
			allNumbers();
		}
		else
		{
			std::swap(*this, b);
			oneRegister();
		}
	}
	else
	{
		if (b.isNumber)
		{
			oneRegister();
		}
		else
		{
			allRegisters();
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
	auto tempStack2 = ptr_64(rsp, 40);

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
			stackChanged = true;
			assembler->bind(foundJump->second);
		}

		auto foundUnwind = unwindMappings.find(code.streamPos);

		if (foundUnwind != unwindMappings.end())
		{
			assembler->bind(foundUnwind->second);
		}

		auto foundDebug = debugMappings.find(code.streamPos);

		if (foundDebug != debugMappings.end())
		{
			assembler->bind(foundDebug->second);
		}

		auto op = code.getValue<BytecodeOp>();

		uint8_t argsSize = code.getValue<uint8_t>();

		if (argsSize)
		{
			code.get(argumentBuffer.data(), argsSize);
		}

		switch (op)
		{
		case ds::BytecodeOp::pushAddr: {
			auto reg = getFreeRegister();
			assembler->lea(reg, ptr_64(functionMappings.at(*(int64_t*)&argumentBuffer[0])));
			compilePushValue(reg);
			break;
		}
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

			if (currentStack.size() && currentStack.rbegin()->size == size)
			{
				(void)compilePopValue(size, true);
			}
			else
			{
				flushStack();
				changeStackBy(-size);
			}
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
			flushStackRegisters();
			assembler->mov(ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, lastStackPos)), rbp);
			assembler->call(functionMappings.at(*(BytecodeOffset*)&argumentBuffer[0]));
			break;
		}
		case ds::BytecodeOp::jump: {
			flushStack();
			flushStackRegisters();
			assembler->jmp(jumpTargetMappings.at(*(BytecodeOffset*)&argumentBuffer[0]));
			break;
		}
		case ds::BytecodeOp::jumpIfNot: {
			auto val = compilePopValue(sizeof(Bool), true);
			flushStackRegisters();
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
			flushStackRegisters();
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
			auto val = compilePopValue(sizeof(Bool), true);
			flushStackRegisters();
			if (val.isNumber)
			{
				bool valueBool = bool(val.number);
				compilePushValue(!valueBool, sizeof(Bool));
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
			flushStackRegisters();
			assembler->mov(ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, lastStackPos)), rbp);
			assembler->mov(argumentRegisters[0], runtimeRegister);
			assembler->call(pointers.at(*(BytecodeOffset*)&argumentBuffer[0]));
			getStack();
			break;
		}
		case ds::BytecodeOp::addInt: {
			auto b = compilePopValue(sizeof(Int), true);
			auto a = compilePopValue(sizeof(Int), true);

			a.compileTwoOp(b, [&a, &b, this] { compilePushValue(a.number + b.number, sizeof(Int)); }, [&a, &b, this] {
				assembler->add(a.gpRegister, b.number);
				compilePushValue(a.gpRegister); }, [&a, &b, this] {
				assembler->add(a.gpRegister, b.gpRegister);
				compilePushValue(a.gpRegister); });
			break;
		}
		case ds::BytecodeOp::subInt: {
			auto b = compilePopValue(sizeof(Int), true);
			auto a = compilePopValue(sizeof(Int), true);

			a.compileTwoOp(b, [&a, &b, this] { compilePushValue(a.number - b.number, sizeof(Int)); }, [&a, &b, this] {
				assembler->sub(a.gpRegister, b.number);
				compilePushValue(a.gpRegister); }, [&a, &b, this] {
				assembler->sub(a.gpRegister, b.gpRegister);
				compilePushValue(a.gpRegister); });
			break;
		}
		case ds::BytecodeOp::mulInt: {
			auto b = compilePopValue(sizeof(Int), true);
			auto a = compilePopValue(sizeof(Int), true);

			a.compileTwoOp(b, [&a, &b, this] { compilePushValue(a.number * b.number, sizeof(Int)); }, [&a, &b, this] {
				assembler->imul(a.gpRegister, b.number);
				compilePushValue(a.gpRegister); }, [&a, &b, this] {
				assembler->imul(a.gpRegister, b.gpRegister);
				compilePushValue(a.gpRegister); });
			break;
		}
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
		case ds::BytecodeOp::addFloat: {
			auto reg = compilePopVec();
			auto reg2 = compilePopVec();
			reg.compileTwoOp(reg2, [&reg, &reg2, this] { compilePushValue(reg.vecNumber + reg2.vecNumber); }, [&reg, &reg2, this] {
				auto toRegister = getFreeVecRegister();
				auto tempRegister = getFreeHalfRegister();
				assembler->mov(tempRegister, reg2.number);
				assembler->movd(toRegister, tempRegister);
				assembler->addss(reg.vecRegister, toRegister);
				compilePushValue(reg.vecRegister); }, [&reg, &reg2, this] {
				assembler->addss(reg.vecRegister, reg2.vecRegister);
				compilePushValue(reg.vecRegister); });
			break;
		}
		case ds::BytecodeOp::subFloat: {
			auto reg2 = compilePopVec();
			auto reg = compilePopVec();
			reg.compileTwoOp(reg2, [&reg, &reg2, this] { compilePushValue(reg.vecNumber - reg2.vecNumber); }, [&reg, &reg2, this] {
				auto toRegister = getFreeVecRegister();
				auto tempRegister = getFreeHalfRegister();
				assembler->mov(tempRegister, reg2.number);
				assembler->movd(toRegister, tempRegister);
				assembler->subss(reg.vecRegister, toRegister);
				compilePushValue(reg.vecRegister); }, [&reg, &reg2, this] {
				assembler->subss(reg.vecRegister, reg2.vecRegister);
				compilePushValue(reg.vecRegister); });
			break;
		}
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
		case ds::BytecodeOp::modFloat: {
			flushStack();
			compilePopValueToRegister(xmm2, true);
			compilePopValueToRegister(xmm3, true);
			assembler->movaps(xmm0, xmm3);
			assembler->divss(xmm3, xmm2);
			assembler->roundss(xmm1, xmm3, 3);
			assembler->mulss(xmm1, xmm2);
			assembler->subss(xmm0, xmm1);
			compilePushValue(xmm0);
			break;
		}
		case ds::BytecodeOp::equalFloat: {
			flushStack();
			auto size = compilePopValueToRegister(xmm1, false);
			assembler->movd(xmm0, ptr_32(stackRegister, -sizeof(Float) - size));
			assembler->ucomiss(xmm0, xmm1);
			assembler->sete(al);
			changeStackBy(-sizeof(Float) - size);
			compilePushValue(al);
			break;
		}
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
			auto b = compilePopValue(sizeof(Int), true);
			auto a = compilePopValue(sizeof(Int), true);
			a.compileTwoOp(b, [&a, &b, this] { compilePushValue(a.number > b.number, sizeof(Bool)); }, [&a, &b, this] {
				assembler->cmp(a.gpRegister, b.number);
				auto result = getFreeByteRegister();
				assembler->setg(result);
				compilePushValue(result); }, [&a, &b, this] {
				assembler->cmp(a.gpRegister, b.gpRegister);
				auto result = getFreeByteRegister();
				assembler->setg(result);
				compilePushValue(result); });
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
			changeStackBy(-int32_t(sizeof(Float)) * 2);
			compilePushValue(al);
			break;
		}
		case ds::BytecodeOp::equals: {
			auto size = *(Size*)&argumentBuffer[0];

			bool isStandardSize = true;
			switch (size)
			{
			case 1:
			case 4:
			case 8:
				break;
			default:
				isStandardSize = false;
				break;
			}

			if (isStandardSize)
			{
				StackValue b = compilePopValue(size, true);
				StackValue a = compilePopValue(size, true);

				// Thank you, John ClangFormat
				// I should seriously think of abandoning it for built in IDE formatting...
				a.compileTwoOp(b, [&a, &b, this] { compilePushValue(a.number == b.number, sizeof(Bool)); }, [&a, &b, this] {
					assembler->cmp(a.gpRegister, b.number);
					auto reg = getFreeByteRegister();
					assembler->sete(reg);
					compilePushValue(reg); }, [&a, &b, this] {
					assembler->cmp(a.gpRegister, b.gpRegister);
					auto reg = getFreeByteRegister();
					assembler->sete(reg);
					compilePushValue(reg); });
			}
			else
			{
				assembler->lea(rsi, ptr_8(stackRegister, -size));
				assembler->lea(rdi, ptr_8(stackRegister, -size * 2));
				assembler->mov(rcx, size);
				assembler->mov(al, 0);
				assembler->stc();
				auto repeatLabel = assembler->new_label();
				assembler->bind(repeatLabel);

				assembler->cmps(ptr_8(rsi), ptr_8(rdi));
				auto endLabel = assembler->new_label();
				assembler->jne(endLabel);
				assembler->dec(rcx);
				assembler->test(rcx, 0);
				assembler->jne(repeatLabel);
				assembler->mov(al, 1);
				assembler->bind(endLabel);
				changeStackBy(size * -2);
				compilePushValue(al);
			}

			break;
		}
		case ds::BytecodeOp::ret:
			flushStack();
			flushStackRegisters();
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
					assembler->mov(ptr_8(variableStackRegister, -offset), popped.number);
					break;
				case 4: {
					assembler->mov(ptr_32(variableStackRegister, -offset), popped.number);
					break;
				}
				case 8:
					assembler->mov(ptr_64(variableStackRegister, -offset), popped.number);
					break;
				default:
					abort();
				}
			}
			else if (popped.isVector)
			{
				assembler->movd(ptr_32(variableStackRegister, -offset), popped.vecRegister);
			}
			else
			{
				switch (size)
				{
				case 1:
					assembler->mov(ptr_8(variableStackRegister, -offset), popped.gpRegister);
					break;
				case 4: {
					assembler->mov(ptr_32(variableStackRegister, -offset), popped.gpRegister);
					break;
				}
				case 8:
					assembler->mov(ptr_64(variableStackRegister, -offset), popped.gpRegister);
					break;
				default:
					assembler->lea(rdi, ptr_64(variableStackRegister, -offset));
					assembler->mov(rsi, popped.gpRegister);
					compileMemoryCopy(size);
					break;
				}
			}
			break;
		}
		case ds::BytecodeOp::readVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];

			Gp target;
			bool isDefaultSize = false;

			switch (size)
			{
			case 1:
				target = getFreeByteRegister();
				assembler->mov(target, ptr_8(variableStackRegister, -offset));
				isDefaultSize = true;
				break;
			case 4: {
				target = getFreeHalfRegister();
				assembler->mov(target, ptr_32(variableStackRegister, -offset));
				isDefaultSize = true;
				break;
			}
			case 8: {
				target = getFreeRegister();
				assembler->mov(target, ptr_64(variableStackRegister, -offset));
				isDefaultSize = true;
				break;
			}
			default:
				flushStack();
				assembler->mov(rdi, stackRegister);
				assembler->lea(rsi, ptr_64(variableStackRegister, -offset));
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
			assembler->add(variableStackRegister, size);
			variableStackChanged = true;
			break;
		}
		case ds::BytecodeOp::popVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			assembler->sub(variableStackRegister, size);
			variableStackChanged = true;
			break;
		}
		case ds::BytecodeOp::allocClass: {
			auto v = compilePopValue(sizeof(Size), true);
			flushStack();

			if (v.isNumber)
			{
				assembler->mov(halfArgumentRegisters[0], v.number);
			}
			else if (!halfArgumentRegisters[0].is_same(v.gpRegister))
			{
				assembler->mov(halfArgumentRegisters[0], v.gpRegister);
			}

			Size typeId = *(Size*)&argumentBuffer[0];
			BytecodeOffset vTableOffset = *(BytecodeOffset*)&argumentBuffer[sizeof(typeId)];
			assembler->mov(halfArgumentRegisters[1], typeId);
			auto offsetPtr = vTableOffset != UINT32_MAX ? (&vTable[vTableOffset]) : nullptr;
			assembler->mov(argumentRegisters[2], offsetPtr);
			assembler->call(RuntimeClass::allocateClass);
			compilePushValue(rax);
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
				assembler->test(rsi, rsi);
				auto endNullCheck = assembler->new_label();
				assembler->jnz(endNullCheck);
				compileAbort("Attempted to read value from a native null reference");
				assembler->bind(endNullCheck);
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
				assembler->test(rdi, rdi);
				auto endNullCheck = assembler->new_label();
				assembler->jnz(endNullCheck);
				compileAbort("Attempted to write value from a native null reference");
				assembler->bind(endNullCheck);
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
			auto val = compilePopValue(sizeof(ds::RuntimeClass*), true);
			// Any compile time known value will not be a valid reference, so probably null.
			if (!val.isNumber)
			{
				flushStack();
				auto nullLabel = assembler->new_label();

				assembler->test(val.gpRegister, val.gpRegister);
				assembler->jz(nullLabel);

				auto ref = [](RuntimeClass* target) {
					target->addRef();
				};
				assembler->mov(tempStack, val.gpRegister);
				if (!val.gpRegister.is_same(argumentRegisters[0]))
				{
					assembler->mov(argumentRegisters[0], val.gpRegister);
				}
				assembler->call((void (*)(RuntimeClass*))ref);
				assembler->mov(val.gpRegister, tempStack);
				assembler->bind(nullLabel);
				compilePushValue(val.gpRegister);
			}
			else
			{
				compilePushValue(val.number, val.size);
			}
			break;
		}
		case ds::BytecodeOp::unrefClass: {
			flushStack();
			flushStackRegisters();
			auto nullLabel = assembler->new_label();
			assembler->mov(rax, ptr_64(stackRegister, -8));
			assembler->mov(ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, lastStackPos)), rbp);

			assembler->test(rax, rax);
			assembler->jz(nullLabel);
			assembler->mov(tempStack, rax);
#if _WIN32
			assembler->lea(argumentRegisters[0], tempStack2);
			assembler->lea(argumentRegisters[1], tempStack);
#else
			assembler->lea(argumentRegisters[1], tempStack2);
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
			getStack();
			assembler->call(r8);
			assembler->jmp(noPopLabel);
			assembler->bind(nativeFunctionLabel);
			assembler->mov(argumentRegisters[0], runtime);
			assembler->call(ptr_64(rax, DS_OFFSETOF(RuntimeFunction, nativeFn)));
			getStack();
			assembler->jmp(noPopLabel);
			assembler->bind(endLabel);
			assembler->bind(nullLabel);
			getStack();
			changeStackBy(-8);
			assembler->bind(noPopLabel);
			break;
		}
		case ds::BytecodeOp::virtualCall: {
			flushStack();
			flushStackRegisters();
			BytecodeOffset called = *(BytecodeOffset*)&argumentBuffer[0];
			assembler->mov(rax, ptr_64(stackRegister, -int32_t(sizeof(Pointer))));

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
			getStack();

			assembler->bind(endLabel);
			break;
		}
		case ds::BytecodeOp::castInterface: {
			auto v = compilePopValue(sizeof(RuntimeClass*), true);

			if (!v.isNumber)
			{
				Int offset = *(Int*)&argumentBuffer[0];
				Bool unCast = *(Bool*)&argumentBuffer[sizeof(offset)];

				if (unCast)
				{
					assembler->sub(v.gpRegister, offset + sizeof(RuntimeClass));
				}
				else
				{
					assembler->add(v.gpRegister, offset + sizeof(RuntimeClass));
				}
				compilePushValue(v.gpRegister);
			}
			else
			{
				compilePushValue(v.number, sizeof(RuntimeClass*));
			}

			break;
		}
		case ds::BytecodeOp::implInterface: {

			flushStack();
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
			assembler->add(rax, offsetBytes);

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
				flushStackRegisters();
				assembler->mov(argumentRegisters[0], runtimeRegister);
				assembler->mov(halfArgumentRegisters[1], size);
				assembler->mov(halfArgumentRegisters[2], offset);
				assembler->mov(halfArgumentRegisters[3], structSize);

				assembler->call(jit_getStructMember);
				getStack();
				break;
			}
			break;
		}
		case ds::BytecodeOp::classIs: {
			auto reg = getFreeRegister();
			compilePopValueToRegister(reg, true);
			flushStack();

			TypeId id = *(TypeId*)&argumentBuffer[0];

			auto nullLabel = assembler->new_label();

			assembler->test(reg, reg);
			assembler->setnz(al);
			assembler->jz(nullLabel);

			assembler->mov(argumentRegisters[1], reg);
			assembler->mov(argumentRegisters[0], runtimeRegister);
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
			auto reg = getFreeRegister();
			compilePopValueToRegister(reg, true);
			flushStack();

			assembler->mov(argumentRegisters[1], reg);
			assembler->mov(argumentRegisters[0], runtimeRegister);
			assembler->mov(argumentRegisters[2], id);
			assembler->call(jit_classAs);
			if (!isNullable)
			{
				assembler->test(rax, rax);

				auto notNullLabel = assembler->new_label();

				assembler->jnz(notNullLabel);
				compileAbort("Non nullable cast failed.");
				assembler->bind(notNullLabel);
			}

			compilePushValue(rax);
			break;
		}
		case ds::BytecodeOp::setStructMember: {
			flushStack();
			flushStackRegisters();
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			Size structSize = *(Size*)&argumentBuffer[sizeof(size) + sizeof(offset)];
			assembler->mov(argumentRegisters[0], runtimeRegister);
			assembler->mov(halfArgumentRegisters[1], size);
			assembler->mov(halfArgumentRegisters[2], offset);
			assembler->mov(halfArgumentRegisters[3], structSize);
			assembler->call(jit_setStructMember);
			getStack();

			changeStackBy(-size);
			break;
		}
		case ds::BytecodeOp::nullCheck: {
			auto v = compilePopValue(sizeof(RuntimeClass*), true);

			if (v.isNumber)
			{
				compileAbort("Attempted to use null reference");
			}
			else
			{
				assembler->test(v.gpRegister, v.gpRegister);
				auto notNullLabel = assembler->new_label();

				assembler->jnz(notNullLabel);
				compileAbort("Attempted to use null reference");
				assembler->bind(notNullLabel);

				compilePushValue(v.gpRegister);
			}
			break;
		}
		case ds::BytecodeOp::awaitTask: {
			Size resultSize = *(Size*)&argumentBuffer[0];
			BytecodeOffset newPos = *(BytecodeOffset*)&argumentBuffer[sizeof(resultSize)];

			auto& foundMapping = jumpTargetMappings.at(newPos);

			compilePopValueToRegister(rax, true);
			changeStackBy(-8);
			flushStackRegisters();
			assembler->mov(r8b, ptr_8(rax, DS_OFFSETOF(modules::system::async::Task, completed) + sizeof(RuntimeClass)));
			assembler->test(r8b, r8b);
			assembler->jnz(foundMapping);

			assembler->mov(argumentRegisters[1], ptr_64(stackRegister));
			assembler->mov(argumentRegisters[0], rax);
			assembler->mov(argumentRegisters[2], runtime);
			assembler->lea(argumentRegisters[3], ptr_64(foundMapping));

			assembler->call(jit_awaitTask);
			getStack();

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

void ds::jit::JustInTimeCompiler::updateDebugOffsets(DebugInfo* debug)
{
	for (auto& i : debug->sections)
	{
		i.offset = Pointer(result->entry) + result->compiled.label_offset(debugMappings.at(i.offset));
	}
}

void ds::jit::JustInTimeCompiler::flushStackRegisters(bool force)
{
	if (stackChanged || force)
	{
		assembler->mov(r10, stackRegister);
		assembler->lea(r11, ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, stack)));
		assembler->sub(r10, r11);
		assembler->mov(stackPos, r10);
		stackChanged = false;
	}
	if (variableStackChanged || force)
	{
		assembler->mov(r10, variableStackRegister);
		assembler->lea(r11, ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, variableStack)));
		assembler->sub(r10, r11);
		assembler->mov(varStackPos, r10);
		variableStackChanged = false;
	}
}

void ds::jit::JustInTimeCompiler::buildPrologCallConventionEntry()
{
	assembler->push(r12);
	assembler->push(r13);
	assembler->push(r14);
	assembler->push(r15);

#if _WIN32 // The windows x64 calling convention makes rsi and rdi nonvolatile, meaning they have to be saved.
	assembler->push(rsi);
	assembler->push(rdi);
#endif
	assembler->push(rbp);

	assembler->mov(rbp, rsp);
	assembler->sub(rsp, 64 + 16);
	assembler->mov(ptr_64(rsp, 64), MANAGED_STACK_BEGIN_MARKER);
}

void ds::jit::JustInTimeCompiler::buildPrologCallConventionExit()
{
	assembler->leave();
#if _WIN32
	assembler->pop(rdi);
	assembler->pop(rsi);
#endif
	assembler->pop(r15);
	assembler->pop(r14);
	assembler->pop(r13);
	assembler->pop(r12);
	assembler->ret();
}

void ds::jit::JustInTimeCompiler::buildProlog()
{
	// Function prolog.
	buildPrologCallConventionEntry();

	assembler->mov(rax, argumentRegisters[0]);
	assembler->mov(runtime, argumentRegisters[1]);

	restoreRegisters(true);
	assembler->call(rax);
	buildPrologCallConventionExit();

	Label resumeLabel = assembler->new_named_label("resume");
	assembler->bind(resumeLabel);
	// Async resume prolog.
	buildPrologCallConventionEntry();
	assembler->mov(rax, argumentRegisters[0]);
	assembler->mov(runtime, argumentRegisters[1]);

	restoreRegisters(true);

	auto resumeProcLabel = assembler->new_label();

	assembler->call(resumeProcLabel);

	buildPrologCallConventionExit();

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
	allocRegister(gpRegister);
	currentStack.push_back(StackValue{ .gpRegister = gpRegister });
}

void ds::jit::JustInTimeCompiler::compilePushValue(asmjit::x86::Vec vecRegister)
{
	allocRegister(vecRegister);
	currentStack.push_back(StackValue{ .isVector = true, .vecRegister = vecRegister });
}

void ds::jit::JustInTimeCompiler::compilePushValue(size_t value, size_t size)
{
	currentStack.push_back(StackValue{ .isNumber = true, .number = value, .size = size });
}

void ds::jit::JustInTimeCompiler::compilePushValue(Float value)
{
	size_t val = 0;
	memcpy(&val, &value, sizeof(Float));
	currentStack.push_back(StackValue{ .isNumber = true, .number = val, .size = sizeof(Float) });
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

void ds::jit::JustInTimeCompiler::restoreRegisters(bool full)
{
	assembler->mov(runtimeRegister, runtime);
	if (full)
	{
		getStack();

		// Restore variable stack
		assembler->mov(variableStackRegister, varStackPos);
		assembler->lea(variableStackRegister, ptr_64(runtimeRegister, variableStackRegister, 0, DS_OFFSETOF(JustInTimeRuntime, variableStack)));
	}
}

void ds::jit::JustInTimeCompiler::getStack()
{
	// Restore stack
	assembler->mov(stackRegister, stackPos);
	assembler->lea(stackRegister, ptr_64(runtimeRegister, stackRegister, 0, DS_OFFSETOF(JustInTimeRuntime, stack)));
	stackChanged = false;
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
	if (!currentStack.empty())
	{
		auto highest = currentStack.rbegin();

		if (highest->isNumber && highest->size != size)
		{
			abort();
		}
		else if (!highest->isVector && !highest->isNumber)
		{
			if (highest->gpRegister.size() != size)
			{
				int s = highest->gpRegister.size();
				throw s;
			}
		}

		StackValue result = *highest;

		currentStack.pop_back();

		return result;
	}

	Gp result;

	switch (size)
	{
	case 1:
		result = getFreeByteRegister();
		assembler->mov(result, ptr_8(stackRegister, -size));
		break;
	case 4: {
		result = getFreeHalfRegister();
		assembler->mov(result, ptr_32(stackRegister, -size));
		break;
	}
	case 8:
		result = getFreeRegister();
		assembler->mov(result, ptr_64(stackRegister, -size));
		break;
	default:
		result = getFreeRegister();
		assembler->lea(result, ptr_64(stackRegister, -size));
		break;
	}
	allocRegister(result);
	if (applyStackPos)
	{
		changeStackBy(-size);
	}
	return StackValue{ .gpRegister = result, .size = size, .stackDiff = size };
}

StackValue ds::jit::JustInTimeCompiler::compilePopVec()
{
	auto v = compilePopValue(sizeof(Float), true);

	if (!v.isNumber && !v.isVector)
	{
		auto reg = getFreeVecRegister();
		allocRegister(reg);
		assembler->movd(reg, v.gpRegister);
		return StackValue{
			.isVector = true,
			.vecRegister = reg,
			.stackDiff = 0,
		};
	}

	return v;
}

int32_t ds::jit::JustInTimeCompiler::compilePopValueToRegister(asmjit::x86::Gp target, bool applyStackPos)
{
	if (!currentStack.empty())
	{
		auto highest = currentStack.rbegin();
		if (highest->isNumber)
		{
			if (highest->size != target.size())
			{
				abort();
			}
			assembler->mov(target, highest->number);
		}
		else
		{
			freeRegister(highest->gpRegister);
			if (highest->gpRegister.size() != target.size())
			{
				abort();
			}
			if (highest->gpRegister != target)
			{
				assembler->mov(target, highest->gpRegister);
			}
		}

		currentStack.pop_back();
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

int32_t ds::jit::JustInTimeCompiler::compilePopValueToRegister(asmjit::x86::Vec target, bool applyStackPos)
{
	if (!currentStack.empty())
	{
		auto highest = currentStack.rbegin();
		if (highest->isNumber)
		{
			assembler->mov(eax, highest->number);
			assembler->movd(target, eax);
		}
		else if (highest->isVector)
		{
			if (highest->vecRegister != target)
			{
				freeRegister(highest->vecRegister);
				assembler->movss(target, highest->vecRegister);
			}
		}
		else
		{
			freeRegister(highest->gpRegister);
			assembler->movd(target, highest->gpRegister);
		}

		currentStack.pop_back();
		return 0;
	}
	assembler->movss(target, ptr_32(stackRegister, -4));

	if (applyStackPos)
	{
		changeStackBy(-4);
	}
	return 4;
}

void ds::jit::JustInTimeCompiler::changeStackBy(int32_t amount)
{
	if (amount == 1)
	{
		assembler->inc(stackRegister);
		stackChanged = true;
	}
	else if (amount == -1)
	{
		assembler->dec(stackRegister);
		stackChanged = true;
	}
	else if (amount > 1)
	{
		assembler->add(stackRegister, amount);
		stackChanged = true;
	}
	else if (amount < -1)
	{
		assembler->sub(stackRegister, -amount);
		stackChanged = true;
	}

#if 0 // stack sanity check
	assembler->lea(r12, ptr_64(runtimeRegister, DS_OFFSETOF(JustInTimeRuntime, stack)));
	assembler->cmp(stackRegister, r12);
	auto okay = assembler->new_label();
	assembler->jge(okay);
	compileAbort("Stack sanity check failed.");
	assembler->bind(okay);
#endif
}

void ds::jit::JustInTimeCompiler::flushStack()
{
	usedTempRegisters.clear();
	usedTempVecRegisters.clear();
	int32_t stackChange = 0;
	for (auto Item = currentStack.begin(); Item != currentStack.end(); Item++)
	{
		StackValue& val = *Item;

		if (val.isNumber)
		{
			switch (val.size)
			{
			case 1:
				assembler->mov(ptr_8(stackRegister, stackChange), val.number);
				break;
			case 4: {
				assembler->mov(ptr_32(stackRegister, stackChange), val.number);
				break;
			}
			case 8:
				assembler->mov(ptr_64(stackRegister, stackChange), val.number);
				break;
			default:
				abort();
			}
			stackChange += val.size;
		}
		else if (val.isVector)
		{
			assembler->movd(ptr_32(stackRegister, stackChange), val.vecRegister);
			stackChange += sizeof(Float);
		}
		else
		{
			switch (val.gpRegister.size())
			{
			case 1:
				assembler->mov(ptr_8(stackRegister, stackChange), val.gpRegister);
				break;
			case 4: {
				assembler->mov(ptr_32(stackRegister, stackChange), val.gpRegister);
				break;
			}
			case 8:
				assembler->mov(ptr_64(stackRegister, stackChange), val.gpRegister);
				break;
			default:
				abort();
			}
			stackChange += val.gpRegister.size();
		}
	}
	changeStackBy(stackChange);
	currentStack.clear();
}

asmjit::x86::Gp ds::jit::JustInTimeCompiler::getFreeRegister()
{
	auto r = getFreeTempRegisterIndex();
	return tempRegisters[r];
}

asmjit::x86::Gp ds::jit::JustInTimeCompiler::getFreeHalfRegister()
{
	auto r = getFreeTempRegisterIndex();
	return tempHalfRegisters[r];
}

asmjit::x86::Gp ds::jit::JustInTimeCompiler::getFreeByteRegister()
{
	auto r = getFreeTempRegisterIndex();
	return tempByteRegisters[r];
}

asmjit::x86::Vec ds::jit::JustInTimeCompiler::getFreeVecRegister()
{
	auto r = getFreeTempVecRegisterIndex();
	return tempVectorRegisters[r];
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

void ds::jit::JustInTimeCompiler::freeRegister(asmjit::x86::Gp& reg)
{
	for (auto& i : usedTempRegisters)
	{
		bool found = false;
		switch (reg.size())
		{
		case 1:
			found = reg.is_same(tempByteRegisters[i]);
			break;
		case 4:
			found = reg.is_same(tempHalfRegisters[i]);
			break;
		case 8:
			found = reg.is_same(tempRegisters[i]);
			break;
		}

		if (found)
		{
			usedTempRegisters.erase(i);
			break;
		}
	}
}

void ds::jit::JustInTimeCompiler::allocRegister(asmjit::x86::Gp& reg)
{
	for (size_t i = 0; i < this->tempRegisters.size(); i++)
	{
		if (usedTempRegisters.contains(i))
		{
			continue;
		}
		bool found = false;
		switch (reg.size())
		{
		case 1:
			found = reg.is_same(tempByteRegisters[i]);
			break;
		case 4:
			found = reg.is_same(tempHalfRegisters[i]);
			break;
		case 8:
			found = reg.is_same(tempRegisters[i]);
			break;
		}

		if (found)
		{
			usedTempRegisters.insert(i);
			break;
		}
	}

	if (usedTempRegisters.size() >= tempRegisters.size())
	{
		flushStack();
	}
}

void ds::jit::JustInTimeCompiler::freeRegister(asmjit::x86::Vec& reg)
{
	for (auto& i : usedTempVecRegisters)
	{
		if (reg.is_same(tempVectorRegisters[i]))
		{
			usedTempVecRegisters.erase(i);
			break;
		}
	}
}

void ds::jit::JustInTimeCompiler::allocRegister(asmjit::x86::Vec& reg)
{
	for (size_t i = 0; i < tempVectorRegisters.size(); i++)
	{
		if (usedTempVecRegisters.contains(i))
		{
			continue;
		}

		if (reg.is_same(tempVectorRegisters[i]))
		{
			usedTempVecRegisters.insert(i);
			return;
		}
	}

	if (usedTempVecRegisters.size() >= usedTempVecRegisters.size())
	{
		flushStack();
	}
}

size_t ds::jit::JustInTimeCompiler::getFreeTempRegisterIndex()
{
	for (size_t i = 0; i < this->tempRegisters.size(); i++)
	{
		if (usedTempRegisters.contains(i))
		{
			continue;
		}
		return i;
	}

	throw "Out of register indices";
}

size_t ds::jit::JustInTimeCompiler::getFreeTempVecRegisterIndex()
{
	for (size_t i = 0; i < tempVectorRegisters.size(); i++)
	{
		if (usedTempVecRegisters.contains(i))
		{
			continue;
		}
		return i;
	}

	throw "Out of vec register indices";
}
