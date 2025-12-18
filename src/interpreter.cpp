#include <ds/interpreter.hpp>
#include <ds/language.hpp>
#include <ds/bytecode.hpp>
#include <ds/class.hpp>
#include <ds/modules/system.async.hpp>
#include <print>
#include <vector>
#include <cstring>

std::mutex threadsMutex;

ds::LanguageRuntime::~LanguageRuntime()
{
	std::map<size_t, std::thread*> threadsCopy;
	{
		std::lock_guard l{ threadsMutex };
		threadsCopy = this->backgroundThreads;
	}
	for (auto& [_, i] : threadsCopy)
	{
		i->join();
	}
}

void ds::LanguageRuntime::defaultCreateBackgroundThread(std::function<void()> f)
{
	static size_t id = 0;

	size_t thisThreadId = id++;

	std::lock_guard l{ threadsMutex };
	auto t = new std::thread([this, f, thisThreadId] {
		f();

		std::lock_guard l{ threadsMutex };
		this->backgroundThreads.erase(thisThreadId);
	});

	backgroundThreads.insert({ thisThreadId, t });
}

ds::LanguageRuntime::LanguageRuntime(LanguageContext* from)
{
	createBackgroundThread = std::bind(&LanguageRuntime::defaultCreateBackgroundThread, this, std::placeholders::_1);
	this->language = from;
	baseContext.runtime = this;
}

void ds::LanguageRuntime::loadBytecode(BytecodeStream* code)
{
	this->bytecodeBuffer = &code->code;
	this->vTable = &code->virtualTable;
	this->debug = &code->debug;
	this->unwindBuffer = code->unwind;

	this->externals.clear();
	this->externals.reserve(code->externalFunctions.size());
	for (auto& i : code->externalFunctions)
	{
		size_t lastColon = i.find_last_of(':');

		std::string first = i.substr(0, lastColon - 1);
		std::string second = i.substr(lastColon + 1);

		bool found = false;
		for (auto& fn : this->language->languageModules[first]->functions)
		{
			if (fn->getFullName() == i)
			{
				this->externals.push_back(fn->function);
				found = true;
				break;
			}
		}

		if (!found)
		{
			std::printf("Could not find function: %s\n", i.c_str());
		}
	}

	this->baseContext.code.buf = this->bytecodeBuffer;
}

void ds::LanguageRuntime::run(bytecodeOffset position)
{
	baseContext.run(position);
}

void ds::InterpretContext::run(bytecodeOffset position)
{
	callStack[callStackPos++] = bytecodeOffset(code.streamPos);
	bytecodeOffset baseCallStackPos = this->callStackPos;
	this->code.streamPos = position;

	runLoop(baseCallStackPos);
	if (callStackPos)
	{
		code.streamPos = callStack[--callStackPos];
	}
}

void ds::InterpretContext::doUnwind()
{
	auto& buffer = runtime->unwindBuffer;

	callStack[callStackPos++] = bytecodeOffset(code.streamPos);

	for (uint32_t i = callStackPos - 1; i > 0; i--)
	{
		bytecodeOffset codePos = callStack[i];

		auto tbl = buffer.getSectionAt(codePos);

		if (!tbl)
		{
			std::cerr << "No unwind info for " << codePos << std::endl;
			continue;
		}

		for (auto p : tbl->parts)
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
}

void ds::InterpretContext::runLoop(bytecodeOffset& baseCallStackPos)
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
			Size size = popValue<Size>();
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
			callStack[callStackPos++] = bytecodeOffset(code.streamPos);
			code.streamPos = size_t(*(bytecodeOffset*)&argumentBuffer[0]);
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
				if (canAwait)
				{
					suspended = true;
					suspendStackPos = baseCallStackPos;
					task->awaiter = this;
					code.streamPos = newPos;
					return;
				}
				auto& rt = this->runtime->asyncContexts.emplace_back();
				rt.copyFrom(this);
				rt.canAwait = true;
				task->awaiter = &rt;
				rt.suspended = true;
				rt.suspendStackPos = callStackPos;
				rt.code.streamPos = newPos;
				pushValue(returnTask);
			}
			break;
		}
		case ds::BytecodeOp::returnAsync: {
			ClassRef<modules::system::async::Task> task = popValue<RuntimeClass*>();
			if (!task->awaiter && !task->awaitNative)
			{
				task.classPtr->addRef();
			}
			pushValue(task);
		}
			[[fallthrough]];
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
			bytecodeOffset vTableOffset = *(bytecodeOffset*)&argumentBuffer[sizeof(typeId)];
			pushValue(RuntimeClass::allocateClass(size, vTableOffset != UINT32_MAX ? runtime->vTable->data() + vTableOffset : nullptr));
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
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			pushBytes(*(uint8_t**)ptr->getBody() + offset, size);
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
					callStack[callStackPos++] = bytecodeOffset(code.streamPos);
					code.streamPos = destructor.codeOffset;
				}
			}

			break;
		}
		case ds::BytecodeOp::concatString: {
			auto second = popRuntimeStringRef();
			auto first = popRuntimeStringRef();

			Size newSize = first.length() + second.length();
			// Space for null terminator
			Size contentSize = newSize + 1;

			RuntimeClass* newClass = RuntimeClass::allocateClass(contentSize + sizeof(uint32_t), 0);

			(*(Size*)newClass->getBody()) = newSize;
			char* strBegin = (char*)(newClass->getBody() + sizeof(uint32_t));
			memcpy(strBegin, first.ptr(), first.length());
			memcpy(strBegin + first.length(), second.ptr(), second.length());
			strBegin[first.length() + second.length()] = 0;
			pushValue<Pointer>(Pointer(newClass));

			break;
		}
		case ds::BytecodeOp::indexString: {
			auto index = popValue<int32_t>();
			auto first = popRuntimeString();
			pushValue<Char>(Char(first.ptr()[index]));
			break;
		}
		case ds::BytecodeOp::setStringIndexCopy: {
			auto index = popValue<int32_t>();
			auto str = popRuntimeString();
			Char newChar = popValue<Char>();
			str.classPtr->addRef();

			// Space for null terminator
			Size strLength = str.length();
			Size contentSize = str.length() + 1;

			RuntimeClass* newClass = RuntimeClass::allocateClass(contentSize + sizeof(Size), 0);

			(*(Size*)newClass->getBody()) = strLength;
			char* strBegin = (char*)(newClass->getBody() + sizeof(Size));
			memcpy(strBegin, str.ptr(), strLength);
			strBegin[index] = newChar;
			std::puts(strBegin);
			pushValue<Pointer>(Pointer(newClass));
			break;
		}
		case ds::BytecodeOp::virtualCall: {
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			bytecodeOffset called = *(bytecodeOffset*)&argumentBuffer[0];
			auto& entry = ptr->vtable[called];
			pushValue(ptr);
			if (entry.nativeFn)
			{
				entry.nativeFn(this);
			}
			else
			{
				callStack[callStackPos++] = bytecodeOffset(code.streamPos);
				code.streamPos = entry.codeOffset;
			}

			break;
		}
		case ds::BytecodeOp::nullCheck: {
			Pointer ptr = popValue<Pointer>();
			if (!ptr)
			{
				runtimePanic(RuntimeStr("Attempted to use null reference"));
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
		default:
			abort();
			break;
		}
	}
}

void ds::InterpretContext::destruct(RuntimeClass* classObject)
{
	auto ptr = RuntimeClass::unref(classObject);

	if (ptr)
	{
		pushValue(classObject);
		virtualCall(ptr);
	}
}

void ds::InterpretContext::copyFrom(InterpretContext* other)
{
	memcpy(this->stack.data(), other->stack.data(), STACK_SIZE);
	this->stackPos = other->stackPos;
	this->variableStack = other->variableStack;
	this->variableStackPos = other->variableStackPos;
	memcpy(this->callStack.data(), other->callStack.data(), other->callStackPos);
	this->callStackPos = other->callStackPos;

	this->code = other->code;
	this->runtime = other->runtime;
}

std::string ds::InterpretContext::popString()
{
	RuntimeClass* ptr = popValue<RuntimeClass*>();

	std::string out = { (const char*)(ptr->getBody() + sizeof(Size)),
		*(Size*)ptr->getBody() };
	RuntimeClass::unref(ptr);
	return out;
}

void ds::InterpretContext::pushRuntimeString(RuntimeStr str)
{
	str.classPtr->addRef();
	pushValue<RuntimeClass*>(str.classPtr);
}

ds::RuntimeStr ds::InterpretContext::popRuntimeString()
{
	return RuntimeStr(popValue<RuntimeClass*>());
}

std::vector<ds::DebugSection*> ds::InterpretContext::getStackTrace() const
{
	std::vector<DebugSection*> result;
	result.push_back(runtime->debug->getSectionAt(uint32_t(code.streamPos)));
	for (uint32_t i = callStackPos - 1; i > 0; i--)
	{
		result.push_back(runtime->debug->getSectionAt(callStack[i]));
	}

	return result;
}

void ds::InterpretContext::virtualCall(RuntimeFunction target)
{
	if (!target)
	{
		return;
	}

	if (target.nativeFn)
	{
		target.nativeFn(this);
	}
	else
	{
		run(target.codeOffset);
	}
}

bool ds::InterpretContext::resumeSuspend()
{
	if (suspended)
	{
		bytecodeOffset pos = suspendStackPos;
		suspended = false;
		runLoop(pos);
		return true;
	}
	return false;
}

void ds::InterpretContext::runtimePanic(RuntimeStr message)
{
	auto stack = getStackTrace();
	std::printf("%s\n", message.ptr());

	for (DebugSection* i : stack)
	{
		if (i)
		{
			std::printf("\t%s()\n", i->name.c_str());
		}
		else
		{
			std::printf("\t<unknown stack frame>\n");
		}
	}
	doUnwind();
}

ds::RuntimeStrRef ds::InterpretContext::popRuntimeStringRef()
{
	return RuntimeStrRef(popValue<RuntimeClass*>());
}
