#include <ds/interpreter.hpp>
#include <ds/bytecode.hpp>
#include <ds/class.hpp>
#include <ds/modules/system.async.hpp>
#include <ds/language.hpp>
#include <vector>
#include <cstring>
#include <cmath>

using namespace ds;

ds::RuntimeInterpretContext::RuntimeInterpretContext(LanguageRuntime* runtime)
{
	this->runtime = runtime;
	this->usedVTable = &runtime->vTable;
}

void ds::RuntimeInterpretContext::run(Pointer position)
{
	callStack[callStackPos++] = BytecodeOffset(code.streamPos);
	BytecodeOffset baseCallStackPos = this->callStackPos;
	this->code.streamPos = position;

	runLoop(baseCallStackPos);
	if (callStackPos)
	{
		code.streamPos = callStack[--callStackPos];
	}
}

bool ds::RuntimeInterpretContext::resumeSuspend()
{
	if (suspended)
	{
		BytecodeOffset pos = suspendStackPos;
		suspended = false;
		runLoop(pos);
		return true;
	}
	return false;
}

void ds::RuntimeInterpretContext::doUnwind()
{
	auto& buffer = runtime->unwindBuffer;

	callStack[callStackPos++] = BytecodeOffset(code.streamPos);

	uint32_t targetPosition = this->suspended ? suspendStackPos : 0;

	for (uint32_t i = callStackPos - 1; i > targetPosition; i--)
	{
		BytecodeOffset codePos = callStack[i];

		auto tbl = buffer.getSectionAt(codePos);

		if (!tbl)
		{
			std::cerr << "No unwind info for " << codePos << std::endl;
			continue;
		}

		for (auto& p : tbl->parts)
		{
			if (p.offset < codePos)
			{
				continue;
			}
			switch (p.op)
			{
			case UnwindOp::popClass: {
				if (p.start > codePos)
				{
					break;
				}
				ds::RuntimeClass* c;
				memcpy(&c, &variableStack[variableStackPos - p.size], sizeof(ds::RuntimeClass*));
				destruct(c);
				break;
			}
			case UnwindOp::popBytes: {
				this->variableStackPos -= p.size;
				break;
			}
			case UnwindOp::pushBytes: {
				this->variableStackPos += p.size;
				break;
			}
			default:
				break;
			}
		}
	}
	callStackPos = 0;
	code.streamPos = SIZE_MAX;
}

void ds::RuntimeInterpretContext::runLoop(BytecodeOffset& baseCallStackPos)
{
	std::array<uint8_t, 255> argumentBuffer{};

	while (!code.empty())
	{
		auto op = code.getValue<BytecodeOp>();

		uint8_t argsSize = code.getValue<uint8_t>();

		if (argsSize)
		{
			code.get(argumentBuffer.data(), argsSize);
		}

		switch (op)
		{
		case ds::BytecodeOp::pushAddr:
		case ds::BytecodeOp::push:
			pushBytes(argumentBuffer.data(), Size(argsSize));
			break;
		case ds::BytecodeOp::pop: {
			stackPos -= *(Size*)&argumentBuffer[0];
			break;
		}
		case ds::BytecodeOp::copy: {
			Size size = *(Size*)&argumentBuffer[0];
			copyBytes(size);
			break;
		}
		case ds::BytecodeOp::jump: {
			code.streamPos = Pointer(*(Size*)&argumentBuffer[0]);
			break;
		}
		case ds::BytecodeOp::jumpIf: {
			Bool cond = popValue<Bool>();
			if (cond)
			{
				code.streamPos = Pointer(*(Size*)&argumentBuffer[0]);
			}
			break;
		}
		case ds::BytecodeOp::jumpIfNot: {
			Bool cond = popValue<Bool>();
			if (!cond)
			{
				code.streamPos = Pointer(*(Size*)&argumentBuffer[0]);
			}
			break;
		}
		case ds::BytecodeOp::addInt:
			pushValue(popValue<Int>() + popValue<Int>());
			break;
		case ds::BytecodeOp::subInt: {
			Int first = popValue<Int>();
			pushValue(popValue<Int>() - first);
			break;
		}
		case ds::BytecodeOp::mulInt:
			pushValue(popValue<Int>() * popValue<Int>());
			break;
		case ds::BytecodeOp::divInt: {
			Int first = popValue<Int>();
			pushValue(popValue<Int>() / first);
			break;
		}
		case ds::BytecodeOp::modInt: {
			Int first = popValue<Int>();
			pushValue(popValue<Int>() % first);
			break;
		}
		case ds::BytecodeOp::negativeInt: {
			pushValue(-popValue<Int>());
			break;
		}
		case ds::BytecodeOp::greaterInt: {
			Int first = popValue<Int>();
			pushValue<Bool>(popValue<Int>() > first);
			break;
		}
		case ds::BytecodeOp::equals: {
			Size size = *(Size*)&argumentBuffer[0];
			Bool same = memcmp(
							&this->stack[stackPos - size],
							&this->stack[stackPos - size * 2], size) == 0;
			stackPos -= size * 2;
			pushValue<Bool>(same);
			break;
		}
		case ds::BytecodeOp::addFloat:
			pushValue(popValue<Float>() + popValue<Float>());
			break;
		case ds::BytecodeOp::subFloat: {
			Float first = popValue<Float>();
			pushValue(popValue<Float>() - first);
			break;
		}
		case ds::BytecodeOp::mulFloat:
			pushValue(popValue<Float>() * popValue<Float>());
			break;
		case ds::BytecodeOp::divFloat: {
			Float first = popValue<Float>();
			pushValue(popValue<Float>() / first);
			break;
		}
		case ds::BytecodeOp::modFloat: {
			Float first = popValue<Float>();
			pushValue(std::fmod(popValue<Float>(), first));
			break;
		}
		case ds::BytecodeOp::equalFloat: {
			pushValue<Bool>(popValue<Float>() == popValue<Float>());
			break;
		}
		case ds::BytecodeOp::negativeFloat: {
			pushValue(-popValue<Float>());
			break;
		}
		case ds::BytecodeOp::greaterFloat: {
			Float first = popValue<Float>();
			pushValue<Bool>(popValue<Float>() > first);
			break;
		}
		case ds::BytecodeOp::intToFloat:
			pushValue(Float(popValue<Int>()));
			break;
		case ds::BytecodeOp::floatToInt:
			pushValue(int32_t(popValue<Float>()));
			break;
		case ds::BytecodeOp::boolNot:
			pushValue<Bool>(!bool(popValue<Bool>()));
			break;
		case ds::BytecodeOp::boolAnd: {
			Bool first = popValue<Bool>();
			Bool second = popValue<Bool>();
			pushValue<Bool>(bool(first && second));
			break;
		}
		case ds::BytecodeOp::boolOr: {
			Bool first = popValue<Bool>();
			Bool second = popValue<Bool>();
			pushValue<Bool>(bool(first || second));
			break;
		}
		case ds::BytecodeOp::call:
			callStack[callStackPos++] = BytecodeOffset(code.streamPos);
			code.streamPos = size_t(*(BytecodeOffset*)&argumentBuffer[0]);
			break;
		case ds::BytecodeOp::callExternal:
			runtime->externals[*(Size*)&argumentBuffer[0]](this);
			break;
		case ds::BytecodeOp::awaitTask: {
			ClassRef<modules::system::async::Task> task = popValue<RuntimeClass*>();
			ClassRef<modules::system::async::Task> returnTask = popValue<RuntimeClass*>();
			Size resultSize = *(Size*)&argumentBuffer[0];
			Pointer newPos = Pointer(*(Size*)&argumentBuffer[sizeof(resultSize)]);
			if (task->completed)
			{
				modules::system::async::pushTaskResult(task.get(), resultSize, this);
				code.streamPos = newPos;
			}
			else
			{
				//if (canAwait)
				//{
				//	suspended = true;
				//	suspendStackPos = baseCallStackPos;
				//	task->awaiter = this;
				//	code.streamPos = newPos;
				//	return;
				//}
				auto& rt = this->runtime->asyncContexts.emplace_back(createSuspendedCopy(callStackPos, newPos));
				task->awaiter = rt;
				pushValue(returnTask);
			}
			break;
		}
		case ds::BytecodeOp::ret:
			if (callStackPos == baseCallStackPos)
			{
				return;
			}
			code.streamPos = callStack[--callStackPos];
			break;
		case ds::BytecodeOp::pushVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			variableStackPos += size;
			break;
		}
		case ds::BytecodeOp::storeVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			popBytes(&variableStack[variableStackPos - offset], size);
			break;
		}
		case ds::BytecodeOp::readVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			pushBytes(&variableStack[variableStackPos - offset], size);
			break;
		}
		case ds::BytecodeOp::popVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			variableStackPos -= size;
			break;
		}
		case ds::BytecodeOp::allocClass: {
			Size size = popValue<Size>();
			Size typeId = *(Size*)&argumentBuffer[0];
			BytecodeOffset vTableOffset = *(BytecodeOffset*)&argumentBuffer[sizeof(typeId)];
			pushValue(RuntimeClass::allocateClass(size, typeId, vTableOffset != UINT32_MAX ? runtime->vTable.data() + vTableOffset : nullptr));
			break;
		}
		case ds::BytecodeOp::implInterface: {
			RuntimeClass* classPtr = popValue<RuntimeClass*>();
			BytecodeOffset offset = *(BytecodeOffset*)&argumentBuffer[0];
			BytecodeOffset vTableOffset = *(BytecodeOffset*)&argumentBuffer[sizeof(offset)];

			RuntimeClass* cls = reinterpret_cast<RuntimeClass*>(classPtr->getBody() + offset);
			*cls = RuntimeClass{
				.vtable = vTableOffset != UINT32_MAX ? runtime->vTable.data() + vTableOffset : nullptr,
				.type = classPtr->type,
				.references = offset,
				.referencesAreOffset = true,
			};

			pushValue(cls);
			break;
		}
		case ds::BytecodeOp::classMember: {
			Size size = popValue<Size>();
			Size offset = popValue<Size>();
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			pushBytes(ptr->getBody() + offset, size);
			break;
		}
		case ds::BytecodeOp::setClassMember: {
			Size size = popValue<Size>();
			Size offset = popValue<Size>();
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			popBytes(ptr->getBody() + offset, size);
			break;
		}
		case ds::BytecodeOp::classMemberPtr: {
			Size size = popValue<Size>();
			Size offset = popValue<Size>();
			RuntimeClass* classPtr = popValue<RuntimeClass*>();
			uint8_t* bodyPointer = *(uint8_t**)classPtr->getBody();
			pushBytes(bodyPointer + offset, size);
			break;
		}
		case ds::BytecodeOp::setClassMemberPtr: {
			Size size = popValue<Size>();
			Size offset = popValue<Size>();
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			popBytes(*(uint8_t**)ptr->getBody() + offset, size);
			break;
		}
		case ds::BytecodeOp::setClassMemberPushAgain: {
			Size size = popValue<Size>();
			Size offset = popValue<Size>();
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			popBytes(ptr->getBody() + offset, size);
			pushValue(ptr);
			break;
		}
		case ds::BytecodeOp::refClass: {
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			if (ptr)
			{
				ptr->addRef();
			}
			pushValue(ptr);
			break;
		}
		case ds::BytecodeOp::unrefClass: {
			auto ptr = popValue<RuntimeClass*>();

			RuntimeFunction destructor = RuntimeClass::unref(ptr);

			if (destructor)
			{
				pushValue(ptr);
				if (destructor.nativeFn)
				{
					destructor.nativeFn(this);
				}
				else
				{
					callStack[callStackPos++] = BytecodeOffset(code.streamPos);
					code.streamPos = destructor.codeOffset;
				}
			}

			break;
		}
		case ds::BytecodeOp::virtualCall: {
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			BytecodeOffset called = *(BytecodeOffset*)&argumentBuffer[0];
			auto& entry = ptr->vtable[called];
			pushValue(ptr);
			if (entry.nativeFn)
			{
				entry.nativeFn(this);
			}
			else
			{
				callStack[callStackPos++] = BytecodeOffset(code.streamPos);
				code.streamPos = entry.codeOffset;
			}

			break;
		}
		case ds::BytecodeOp::nullCheck: {
			Pointer ptr = popValue<Pointer>();
			if (!ptr) [[unlikely]]
			{
				runtimePanic("Attempted to use null reference");
				return;
			}
			pushValue(ptr);
			break;
		}
		case ds::BytecodeOp::getStructMember: {
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			Size structSize = *(Size*)&argumentBuffer[sizeof(size) + sizeof(offset)];
			auto targetPos = stackPos - offset - size;
			stackPos -= structSize;
			pushBytes(&stack[targetPos], size);
			break;
		}
		case ds::BytecodeOp::setStructMember: {
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			Size structSize = *(Size*)&argumentBuffer[sizeof(size) + sizeof(offset)];
			auto targetPos = stackPos - offset - size;
			memcpy(&stack[targetPos], &this->stack[stackPos - structSize - size], size);
			memmove(&stack[stackPos - structSize - size], &this->stack[stackPos - structSize], structSize);
			stackPos -= size;
			break;
		}
		case ds::BytecodeOp::suspend: {
			suspended = true;
			suspendStackPos = baseCallStackPos;
			return;
		}
		case ds::BytecodeOp::unwind: {
			doUnwind();
			return;
		}
		case ds::BytecodeOp::classIs: {
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			if (ptr)
			{
				TypeId id = *(TypeId*)&argumentBuffer[0];
				if (id == ptr->type || this->runtime->reflect->isSubclassOf(ptr->type, id))
				{
					pushValue<Bool>(true);
				}
				else
				{
					pushValue<Bool>(false);
				}
			}
			else
			{
				pushValue<Bool>(false);
			}
			break;
		}
		case ds::BytecodeOp::classAs: {
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			Bool isNullable = *(Bool*)&argumentBuffer[sizeof(TypeId)];
			if (ptr)
			{
				TypeId id = *(TypeId*)&argumentBuffer[0];
				if (id != ptr->type)
				{
					auto [success, offset] = this->runtime->reflect->tryCast(ptr->type, id);

					if (success)
					{
						ptr = reinterpret_cast<RuntimeClass*>(ptr->getBody() + offset);
					}
					else
					{
						ptr = nullptr;
					}
				}
			}
			if (!isNullable && !ptr)
			{
				runtimePanic("Non nullable cast failed.");
			}
			pushValue(ptr);
			break;
		}
		case ds::BytecodeOp::noReturn: {
			runtimePanic("Function did not return");
			break;
		}
		case ds::BytecodeOp::castInterface: {
			Int offset = *(Int*)&argumentBuffer[0];
			Bool unCast = *(Bool*)&argumentBuffer[sizeof(offset)];
			RuntimeClass* classPtr = popValue<RuntimeClass*>();
			if (unCast)
			{
				classPtr = (RuntimeClass*)((uint8_t*)classPtr - offset - sizeof(RuntimeClass));
			}
			else
			{
				classPtr = (RuntimeClass*)((uint8_t*)classPtr + offset + sizeof(RuntimeClass));
			}
			pushValue(classPtr);
			break;
		}
		default:
			abort();
			break;
		}
	}
}

void ds::RuntimeInterpretContext::loadBytecode(BytecodeStream* code)
{
	runtime->bytecodeBuffer = &code->code;
	runtime->debug = &code->debug;
	runtime->unwindBuffer = code->unwind;
	runtime->reflect = &code->reflect;

	runtime->externals.clear();
	runtime->externals.reserve(code->externalFunctions.size());
	for (auto& i : code->externalFunctions)
	{
		size_t lastColon = i.find_last_of(':');

		std::string first = i.substr(0, lastColon - 1);
		std::string second = i.substr(lastColon + 1);

		bool found = false;
		for (auto& fn : runtime->language->languageModules[first]->getFunctions())
		{
			if (fn->getFullName() == i)
			{
				runtime->externals.push_back(fn->function);
				found = true;
				break;
			}
		}

		if (!found)
		{
			std::printf("Could not find function: %s\n", i.c_str());
		}
	}

	runtime->vTable.clear();
	runtime->vTable.reserve(code->virtualTable.size());
	for (auto& [offset, native] : code->virtualTable)
	{
		if (native)
		{
			runtime->vTable.push_back(RuntimeFunction{
				.nativeFn = runtime->externals[native] });
		}
		else
		{
			runtime->vTable.push_back(RuntimeFunction{
				.codeOffset = offset });
		}
	}

	this->code.buf = runtime->bytecodeBuffer;
}

InterpretContext* ds::RuntimeInterpretContext::createCopy()
{
	auto other = new RuntimeInterpretContext(runtime);
	if (stackPos)
	{
		memcpy(other->stack.data(), stack.data(), stackPos);
	}
	other->stackPos = stackPos;
	if (variableStackPos)
	{
		memcpy(other->variableStack.data(), variableStack.data(), variableStackPos);
	}
	other->variableStackPos = variableStackPos;
	if (callStackPos)
	{
		memcpy(other->callStack.data(), callStack.data(), callStackPos * sizeof(BytecodeOffset));
	}
	other->callStackPos = callStackPos;
	other->code = code;
	return other;

}
InterpretContext* RuntimeInterpretContext::createSuspendedCopy(BytecodeOffset stackOffset, size_t streamPosition)
{
	auto other = reinterpret_cast<RuntimeInterpretContext*>(createCopy());
	other->suspended = true;
	other->suspendStackPos = stackOffset;
	other->code.streamPos = streamPosition;
	other->canAwait = true;
	return other;
}

std::vector<ds::DebugSection*> ds::RuntimeInterpretContext::getStackTrace() const
{
	std::vector<DebugSection*> result;
	result.push_back(runtime->debug->getSectionAt(uint32_t(code.streamPos)));
	for (uint32_t i = callStackPos - 1; i > 0; i--)
	{
		result.push_back(runtime->debug->getSectionAt(callStack[i]));
	}

	return result;
}
