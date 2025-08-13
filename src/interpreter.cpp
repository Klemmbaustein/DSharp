#include "interpreter.hpp"
#include <language.hpp>
#include <bytecode.hpp>
#include <class.hpp>
#include <print>
#include <vector>

lang::InterpretContext::InterpretContext(LanguageContext* from)
{
	this->language = from;
}

void lang::InterpretContext::loadBytecode(BytecodeStream* code)
{
	this->bytecodeBuffer = &code->code;
	this->vTable = &code->virtualTable;
	this->debug = &code->debug;

	this->externals.reserve(code->externalFunctions.size());
	for (auto& i : code->externalFunctions)
	{
		size_t lastColon = i.find_last_of(':');

		std::string first = i.substr(0, lastColon - 1);
		std::string second = i.substr(lastColon + 1);

		bool found = false;
		for (auto& i : this->language->languageModules[first]->functions)
		{
			if (i->name == second)
			{
				this->externals.push_back(i->function);
			}
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
			pushBytes(argumentBuffer.data(), size_t(argsSize));
			break;
		case lang::BytecodeOp::pop: {
			stackPos -= *(uint32_t*)&argumentBuffer[0];
			break;
		}
		case lang::BytecodeOp::copy: {
			uint32_t size = *(uint32_t*)&argumentBuffer[0];
			copyBytes(size);
			break;
		}
		case lang::BytecodeOp::jump: {
			bytecodeBuffer->streamPos = size_t(*(uint32_t*)&argumentBuffer[0]);
			break;
		}
		case lang::BytecodeOp::jumpIfNot: {
			bool cond = popValue<bool>();
			if (!cond)
			{
				bytecodeBuffer->streamPos = size_t(*(uint32_t*)&argumentBuffer[0]);
			}
			break;
		}

		case lang::BytecodeOp::addInt:
			pushValue(popValue<int32_t>() + popValue<int32_t>());
			break;
		case lang::BytecodeOp::subInt: {
			int32_t first = popValue<int32_t>();
			pushValue(popValue<int32_t>() - first);
			break;
		}
		case lang::BytecodeOp::mulInt:
			pushValue(popValue<int32_t>() * popValue<int32_t>());
			break;
		case lang::BytecodeOp::divInt: {
			int32_t first = popValue<int32_t>();
			pushValue(popValue<int32_t>() / first);
			break;
		}
		case lang::BytecodeOp::greaterInt: {
			int32_t first = popValue<int32_t>();
			pushValue(popValue<int32_t>() > first);
			break;
		}
		case lang::BytecodeOp::equals: {
			uint32_t size = popValue<uint32_t>();
			bool same = memcmp(
							&this->stack[stackPos - size],
							&this->stack[stackPos - size * 2], size) == 0;
			stackPos -= size * 2;
			pushValue<bool>(same);
			break;
		}
		case lang::BytecodeOp::addFloat:
			pushValue(popValue<float>() + popValue<float>());
			break;
		case lang::BytecodeOp::subFloat: {
			float first = popValue<float>();
			pushValue(popValue<float>() - first);
			break;
		}
		case lang::BytecodeOp::mulFloat:
			pushValue(popValue<float>() * popValue<float>());
			break;
		case lang::BytecodeOp::divFloat: {
			float first = popValue<float>();
			pushValue(popValue<float>() / first);
			break;
		}
		case lang::BytecodeOp::greaterFloat: {
			float first = popValue<float>();
			pushValue(popValue<float>() > first);
			break;
		}
		case lang::BytecodeOp::intToFloat:
			pushValue(float(popValue<int32_t>()));
			break;
		case lang::BytecodeOp::floatToInt:
			pushValue(int32_t(popValue<float>()));
			break;
		case lang::BytecodeOp::boolNot:
			pushValue(!popValue<bool>());
			break;
		case lang::BytecodeOp::boolAnd: {
			bool first = popValue<bool>();
			bool second = popValue<bool>();
			pushValue(first && second);
			break;
		}
		case lang::BytecodeOp::call:
			callStack[callStackPos++] = bytecodeOffset(bytecodeBuffer->streamPos);
			bytecodeBuffer->streamPos = size_t(*(bytecodeOffset*)&argumentBuffer[0]);
			break;
		case lang::BytecodeOp::callExternal:
			externals[*(uint32_t*)&argumentBuffer[0]](this);
			break;
		case lang::BytecodeOp::ret:
			if (callStackPos == 0)
			{
				std::print("program returned: '{}'\n", popValue<int32_t>());
				return;
			}
			else if (callStackPos == baseCallStackPos)
			{
				return;
			}
			bytecodeBuffer->streamPos = callStack[--callStackPos];
			break;
		case lang::BytecodeOp::pushVariable: {
			uint32_t size = *(uint32_t*)&argumentBuffer[0];
			variableStackPos += size;
			break;
		}
		case lang::BytecodeOp::storeVariable: {
			uint32_t size = *(uint32_t*)&argumentBuffer[0];
			uint32_t offset = *(uint32_t*)&argumentBuffer[sizeof(size)];
			popBytes(&variableStack[variableStackPos - offset], size);
			break;
		}
		case lang::BytecodeOp::readVariable: {
			uint32_t size = *(uint32_t*)&argumentBuffer[0];
			uint32_t offset = *(uint32_t*)&argumentBuffer[sizeof(size)];
			pushBytes(&variableStack[variableStackPos - offset], size);
			break;
		}
		case lang::BytecodeOp::popVariable: {
			uint32_t size = *(uint32_t*)&argumentBuffer[0];
			variableStackPos -= size;
			break;
		}
		case lang::BytecodeOp::allocClass: {
			uint32_t size = popValue<uint32_t>();
			uint32_t typeId = *(uint32_t*)&argumentBuffer[0];
			bytecodeOffset vTableOffset = *(bytecodeOffset*)&argumentBuffer[sizeof(typeId)];
			pushValue(RuntimeClass::allocateClass(size, vTableOffset != UINT32_MAX ? vTable->data() + vTableOffset : nullptr));
			break;
		}
		case lang::BytecodeOp::classMember: {
			uint32_t size = popValue<uint32_t>();
			uint32_t offset = popValue<uint32_t>();
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			pushBytes(ptr->getBody() + offset, size);
			break;
		}
		case lang::BytecodeOp::setClassMember: {
			uint32_t size = popValue<uint32_t>();
			uint32_t offset = popValue<uint32_t>();
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			popBytes(ptr->getBody() + offset, size);
			break;
		}
		case lang::BytecodeOp::setClassMemberPushAgain: {
			uint32_t size = popValue<uint32_t>();
			uint32_t offset = popValue<uint32_t>();
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

			uint32_t newSize = first.length() + second.length();
			// Space for null terminator
			uint32_t contentSize = newSize + 1;

			RuntimeClass* newClass = RuntimeClass::allocateClass(contentSize + sizeof(uint32_t), 0);

			(*(uint32_t*)newClass->getBody()) = newSize;
			char* strBegin = (char*)(newClass->getBody() + sizeof(uint32_t));
			memcpy(strBegin, first.ptr(), first.length());
			memcpy(strBegin + first.length(), second.ptr(), second.length());
			strBegin[first.length() + second.length()] = 0;
			pushValue<size_t>(size_t(newClass));

			break;
		}
		case lang::BytecodeOp::indexString: {
			auto index = popValue<int32_t>();
			auto first = popRuntimeString();
			pushValue<char>(char(first.ptr()[index]));
			break;
		}
		case lang::BytecodeOp::setStringIndexCopy: {
			auto index = popValue<int32_t>();
			auto str = popRuntimeString();
			char newChar = popValue<char>();

			// Space for null terminator
			uint32_t strLength = str.length();
			uint32_t contentSize = str.length() + 1;

			RuntimeClass* newClass = RuntimeClass::allocateClass(contentSize + sizeof(uint32_t), 0);

			(*(uint32_t*)newClass->getBody()) = strLength;
			char* strBegin = (char*)(newClass->getBody() + sizeof(uint32_t));
			memcpy(strBegin, str.ptr(), strLength);
			strBegin[index] = newChar;
			pushValue<size_t>(size_t(newClass));
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
			size_t ptr = popValue<size_t>();
			if (!ptr)
			{
				runtimePanic(RuntimeStr("Attempted to use null reference"));

				abort();
			}
			pushValue(ptr);
			break;
		}
		default:
			abort();
			break;
		}
	}
}

std::string lang::InterpretContext::popString()
{
	RuntimeClass* ptr = popValue<RuntimeClass*>();

	std::string out = { (const char*)(ptr->getBody() + sizeof(uint32_t)),
		*(uint32_t*)ptr->getBody() };
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
	result.push_back(this->debug->getSectionAt(bytecodeBuffer->streamPos));
	for (int i = callStackPos - 1; i >= 0; i--)
	{
		result.push_back(this->debug->getSectionAt(callStack[i]));
	}

	return result;
}

void lang::InterpretContext::virtualCall(VTableEntry target)
{
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
	std::println("{}", std::string(message.ptr(), message.length()));

	for (DebugSection* i : stack)
	{
		if (i)
		{
			std::println("\t{}()", i->name);
		}
		else
		{
			std::println("\t<unknown stack frame>");
		}
	}
	abort();
}

lang::RuntimeStrRef lang::InterpretContext::popRuntimeStringRef()
{
	return RuntimeStrRef(popValue<RuntimeClass*>());
}
