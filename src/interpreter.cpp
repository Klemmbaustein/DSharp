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

	this->externals.reserve(code->externalFunctions.size());
	for (auto& i : code->externalFunctions)
	{
		size_t lastColon = i.find_last_of(':');

		std::string first = i.substr(0, lastColon - 1);
		std::string second = i.substr(lastColon + 1);

		bool found = false;
		for (auto& i : this->language->languageModules[first]->functions)
		{
			if (i.name == second)
			{
				this->externals.push_back(i.function);
			}
		}
	}
}

void lang::InterpretContext::run()
{
	std::array<uint8_t, 255> argumentBuffer{};

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
		case lang::BytecodeOp::lessInt: {
			int32_t first = popValue<int32_t>();
			pushValue(popValue<int32_t>() < first);
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
		case lang::BytecodeOp::intToFloat:
			pushValue(float(popValue<int32_t>()));
			break;
		case lang::BytecodeOp::floatToInt:
			pushValue(int32_t(popValue<float>()));
			break;
		case lang::BytecodeOp::call:
			functionStack[functionStackPos++] = bytecodeOffset(bytecodeBuffer->streamPos);
			bytecodeBuffer->streamPos = size_t(*(bytecodeOffset*)&argumentBuffer[0]);
			break;
		case lang::BytecodeOp::callExternal:
			externals[*(uint32_t*)&argumentBuffer[0]](this);
			break;
		case lang::BytecodeOp::ret:
			if (functionStackPos == 0)
			{
				std::print("program returned: {}\n", popValue<int>());
				return;
			}
			bytecodeBuffer->streamPos = functionStack[--functionStackPos];
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
			uint32_t destructor = *(uint32_t*)&argumentBuffer[sizeof(typeId)];
			pushValue(RuntimeClass::allocateClass(size, destructor));
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
			RuntimeClass* ptr = popValue<RuntimeClass*>();
			uint32_t offset = *(uint32_t*)&argumentBuffer[0];
			uint32_t size = *(uint32_t*)&argumentBuffer[sizeof(offset)];
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
			ptr->addRef();
			pushValue(ptr);
			break;
		}
		case lang::BytecodeOp::unrefClass: {
			auto ptr = popValue<RuntimeClass*>();
			bytecodeOffset destructor = RuntimeClass::unref(ptr);

			if (destructor != 0 && destructor != UINT32_MAX)
			{
				pushValue(ptr);
				functionStack[functionStackPos++] = bytecodeOffset(bytecodeBuffer->streamPos);
				bytecodeBuffer->streamPos = destructor;
			}

			break;
		}
		case lang::BytecodeOp::concatString: {
			auto second = popStringLength();
			auto first = popStringLength();

			uint32_t newSize = first.length + second.length + 1;

			RuntimeClass* newClass = RuntimeClass::allocateClass(newSize + sizeof(uint32_t), 0);

			(*(uint32_t*)newClass->getBody()) = newSize;
			char* strBegin = (char*)(newClass->getBody() + sizeof(uint32_t));
			memcpy(strBegin, first.ptr, first.length);
			memcpy(strBegin + first.length, second.ptr, second.length);
			strBegin[first.length + second.length] = 0;
			pushValue<size_t>(size_t(newClass));

			break;
		}

		default:
			abort();
			break;
		}
	}
}

const char* lang::InterpretContext::popString()
{
	RuntimeClass* ptr = popValue<RuntimeClass*>();

	return (const char*)(ptr->getBody() + sizeof(uint32_t));
}

lang::RuntimeStr lang::InterpretContext::popStringLength()
{
	RuntimeClass* ptr = popValue<RuntimeClass*>();

	return { (const char*)(ptr->getBody() + sizeof(uint32_t)), *(uint32_t*)ptr->getBody() };
}
