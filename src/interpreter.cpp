#include "interpreter.hpp"
#include <language.hpp>
#include <bytecode.hpp>
#include <class.hpp>
#include <print>
#include <vector>
#include <cstring>

lang::InterpretContext::InterpretContext(LanguageContext* from)
{
	this->language = from;
}

void lang::InterpretContext::loadBytecode(BytecodeStream* code)
{
	this->bytecodeBuffer = &code->code;
	this->vTable = &code->virtualTable;
	this->debug = &code->debug;
	this->callStackPos = 0;
	this->stackPos = 0;
	this->variableStackPos = 0;

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
}

void lang::InterpretContext::run(bytecodeOffset position)
{
	std::array<uint8_t, 255> argumentBuffer{};

	bytecodeOffset baseCallStackPos = this->callStackPos;
	bytecodeBuffer->streamPos = position;

	while (!bytecodeBuffer->empty())
	{
		auto op = bytecodeBuffer->getValue<BytecodeOp>();

		uint8_t argsSize = bytecodeBuffer->getValue<uint8_t>();

		if (argsSize)
		{
			bytecodeBuffer->get(argumentBuffer.data(), argsSize);
		}

		switch (op)
		{
		case lang::BytecodeOp::push:
			pushBytes(argumentBuffer.data(), Size(argsSize));
			break;
		case lang::BytecodeOp::pop: {
			stackPos -= *(Size*)&argumentBuffer[0];
			break;
		}
		case lang::BytecodeOp::copy: {
			Size size = *(Size*)&argumentBuffer[0];
			copyBytes(size);
			break;
		}
		case lang::BytecodeOp::jump: {
			bytecodeBuffer->streamPos = Pointer(*(Size*)&argumentBuffer[0]);
			break;
		}
		case lang::BytecodeOp::jumpIfNot: {
			Bool cond = popValue<Bool>();
			if (!cond)
			{
				bytecodeBuffer->streamPos = Pointer(*(Size*)&argumentBuffer[0]);
			}
			break;
		}

		case lang::BytecodeOp::addInt:
			pushValue(popValue<Int>() + popValue<Int>());
			break;
		case lang::BytecodeOp::subInt: {
			Int first = popValue<Int>();
			pushValue(popValue<Int>() - first);
			break;
		}
		case lang::BytecodeOp::mulInt:
			pushValue(popValue<Int>() * popValue<Int>());
			break;
		case lang::BytecodeOp::divInt: {
			Int first = popValue<Int>();
			pushValue(popValue<Int>() / first);
			break;
		}
		case lang::BytecodeOp::greaterInt: {
			Int first = popValue<Int>();
			pushValue<Bool>(popValue<Int>() > first);
			break;
		}
		case lang::BytecodeOp::equals: {
			Size size = popValue<Size>();
			Bool same = memcmp(
							&this->stack[stackPos - size],
							&this->stack[stackPos - size * 2], size) == 0;
			stackPos -= size * 2;
			pushValue<Bool>(same);
			break;
		}
		case lang::BytecodeOp::addFloat:
			pushValue(popValue<Float>() + popValue<Float>());
			break;
		case lang::BytecodeOp::subFloat: {
			Float first = popValue<Float>();
			pushValue(popValue<Float>() - first);
			break;
		}
		case lang::BytecodeOp::mulFloat:
			pushValue(popValue<Float>() * popValue<Float>());
			break;
		case lang::BytecodeOp::divFloat: {
			Float first = popValue<Float>();
			pushValue(popValue<Float>() / first);
			break;
		}
		case lang::BytecodeOp::greaterFloat: {
			Float first = popValue<Float>();
			pushValue<Bool>(popValue<Float>() > first);
			break;
		}
		case lang::BytecodeOp::intToFloat:
			pushValue(Float(popValue<Int>()));
			break;
		case lang::BytecodeOp::floatToInt:
			pushValue(int32_t(popValue<Float>()));
			break;
		case lang::BytecodeOp::boolNot:
			pushValue<Bool>(!bool(popValue<Bool>()));
			break;
		case lang::BytecodeOp::boolAnd: {
			Bool first = popValue<Bool>();
			Bool second = popValue<Bool>();
			pushValue<Bool>(bool(first && second));
			break;
		}
		case lang::BytecodeOp::boolOr: {
			Bool first = popValue<Bool>();
			Bool second = popValue<Bool>();
			pushValue<Bool>(bool(first || second));
			break;
		}
		case lang::BytecodeOp::call:
			callStack[callStackPos++] = bytecodeOffset(bytecodeBuffer->streamPos);
			bytecodeBuffer->streamPos = size_t(*(bytecodeOffset*)&argumentBuffer[0]);
			break;
		case lang::BytecodeOp::callExternal:
			externals[*(Size*)&argumentBuffer[0]](this);
			break;
		case lang::BytecodeOp::ret:
			if (callStackPos == baseCallStackPos)
			{
				return;
			}
			bytecodeBuffer->streamPos = callStack[--callStackPos];
			break;
		case lang::BytecodeOp::pushVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			variableStackPos += size;
			break;
		}
		case lang::BytecodeOp::storeVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			popBytes(&variableStack[variableStackPos - offset], size);
			break;
		}
		case lang::BytecodeOp::readVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			pushBytes(&variableStack[variableStackPos - offset], size);
			break;
		}
		case lang::BytecodeOp::popVariable: {
			Size size = *(Size*)&argumentBuffer[0];
			variableStackPos -= size;
			break;
		}
		case lang::BytecodeOp::allocClass: {
			Size size = popValue<Size>();
			Size typeId = *(Size*)&argumentBuffer[0];
			bytecodeOffset vTableOffset = *(bytecodeOffset*)&argumentBuffer[sizeof(typeId)];
			pushValue(RuntimeClass::allocateClass(size, vTableOffset != UINT32_MAX ? vTable->data() + vTableOffset : nullptr));
			break;
		}
		case lang::BytecodeOp::classMember: {
			Size size = popValue<Size>();
			Size offset = popValue<Size>();
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			pushBytes(ptr->getBody() + offset, size);
			break;
		}
		case lang::BytecodeOp::setClassMember: {
			Size size = popValue<Size>();
			Size offset = popValue<Size>();
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			popBytes(ptr->getBody() + offset, size);
			break;
		}
		case lang::BytecodeOp::setClassMemberPushAgain: {
			Size size = popValue<Size>();
			Size offset = popValue<Size>();
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			popBytes(ptr->getBody() + offset, size);
			pushValue(ptr);
			break;
		}
		case lang::BytecodeOp::refClass: {
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			if (ptr)
			{
				ptr->addRef();
			}
			pushValue(ptr);
			break;
		}
		case lang::BytecodeOp::unrefClass: {
			auto ptr = popValue<RuntimeClass*>();

			VTableEntry destructor = RuntimeClass::unref(ptr);

			if (destructor)
			{
				pushValue(ptr);
				if (destructor.nativeFn)
				{
					destructor.nativeFn(this);
				}
				else
				{
					callStack[callStackPos++] = bytecodeOffset(bytecodeBuffer->streamPos);
					bytecodeBuffer->streamPos = destructor.codeOffset;
				}
			}

			break;
		}
		case lang::BytecodeOp::concatString: {
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
		case lang::BytecodeOp::indexString: {
			auto index = popValue<int32_t>();
			auto first = popRuntimeString();
			pushValue<Char>(Char(first.ptr()[index]));
			break;
		}
		case lang::BytecodeOp::setStringIndexCopy: {
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
		case lang::BytecodeOp::virtualCall: {
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
				callStack[callStackPos++] = bytecodeOffset(bytecodeBuffer->streamPos);
				bytecodeBuffer->streamPos = entry.codeOffset;
			}

			break;
		}
		case lang::BytecodeOp::nullCheck: {
			Pointer ptr = popValue<Pointer>();
			if (!ptr)
			{
				runtimePanic(RuntimeStr("Attempted to use null reference"));
			}
			pushValue(ptr);
			break;
		}
		case lang::BytecodeOp::getStructMember: {
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			Size structSize = *(Size*)&argumentBuffer[sizeof(size) + sizeof(offset)];
			auto targetPos = stackPos - offset - size;
			stackPos -= structSize;
			pushBytes(&stack[targetPos], size);
			break;
		}
		case lang::BytecodeOp::setStructMember: {
			Size size = *(Size*)&argumentBuffer[0];
			Size offset = *(Size*)&argumentBuffer[sizeof(size)];
			Size structSize = *(Size*)&argumentBuffer[sizeof(size) + sizeof(offset)];
			auto targetPos = stackPos - structSize + offset;
			memcpy(&stack[targetPos], &this->stack[stackPos - structSize - size], size);
			memmove(&stack[stackPos - structSize - size], &this->stack[stackPos - structSize], structSize);
			stackPos -= size;
			break;
		}
		default:
			abort();
			break;
		}
	}
}

void lang::InterpretContext::destruct(RuntimeClass* classObject)
{
	auto ptr = RuntimeClass::unref(classObject);

	if (ptr)
	{
		pushValue(classObject);
		virtualCall(ptr);
	}
}

std::string lang::InterpretContext::popString()
{
	RuntimeClass* ptr = popValue<RuntimeClass*>();

	std::string out = { (const char*)(ptr->getBody() + sizeof(Size)),
		*(Size*)ptr->getBody() };
	RuntimeClass::unref(ptr);
	return out;
}

void lang::InterpretContext::pushRuntimeString(RuntimeStr str)
{
	str.classPtr->addRef();
	pushValue<RuntimeClass*>(str.classPtr);
}

lang::RuntimeStr lang::InterpretContext::popRuntimeString()
{
	return RuntimeStr(popValue<RuntimeClass*>());
}

std::vector<lang::DebugSection*> lang::InterpretContext::getStackTrace() const
{
	std::vector<DebugSection*> result;
	result.push_back(this->debug->getSectionAt(uint32_t(bytecodeBuffer->streamPos)));
	for (int i = callStackPos - 1; i >= 0; i--)
	{
		result.push_back(this->debug->getSectionAt(callStack[i]));
	}

	return result;
}

void lang::InterpretContext::virtualCall(VTableEntry target)
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
		callStack[callStackPos++] = bytecodeOffset(bytecodeBuffer->streamPos);
		run(target.codeOffset);
		bytecodeBuffer->streamPos = callStack[--callStackPos];
	}
}

void lang::InterpretContext::runtimePanic(RuntimeStr message) const
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
	abort();
}

lang::RuntimeStrRef lang::InterpretContext::popRuntimeStringRef()
{
	return RuntimeStrRef(popValue<RuntimeClass*>());
}
