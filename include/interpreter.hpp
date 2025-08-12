#pragma once
#include "bytecode.hpp"
#include <array>
#include <native/externalFunction.hpp>
#include "runtimeString.hpp"

namespace lang
{
	struct LanguageContext;

	class InterpretContext
	{
	public:
		InterpretContext(LanguageContext* from);

		void loadBytecode(BytecodeStream* code);

		void run(bytecodeOffset position = 0);

		template <typename T>
		T popValue()
		{
			stackPos -= sizeof(T);
			return *(T*)&this->stack[stackPos];
		}

		std::string popString();
		RuntimeStr popRuntimeString();
		void pushRuntimeString(RuntimeStr str);

		template <typename T>
		void pushValue(const T& value)
		{
			*((T*)&this->stack[stackPos]) = value;
			stackPos += sizeof(value);
		}

		void pushBytes(const uint8_t* data, size_t size)
		{
			if (size == 0)
				return;

			memcpy(&this->stack[stackPos], data, size);
			stackPos += size;
		}
		void popBytes(uint8_t* to, size_t size)
		{
			if (size == 0)
				return;

			stackPos -= size;
			memcpy(to, &this->stack[stackPos], size);
		}
		void copyBytes(size_t size)
		{
			if (size == 0)
				return;

			memcpy(&this->stack[stackPos], &this->stack[stackPos - size], size);
			stackPos += size;
		}

		uint32_t getVarArgsCount()
		{
			return popValue<uint32_t>();
		}

		std::vector<DebugSection*> getStackTrace() const;

		void virtualCall(VTableEntry target);

		void runtimePanic(RuntimeStr message) const;

	private:
		constexpr static size_t STACK_SIZE = 8000;
		constexpr static size_t CALL_STACK_SIZE = 512;
		RuntimeStrRef popRuntimeStringRef();

		LanguageContext* language = nullptr;

		std::vector<ExternalFunctionPointer> externals;

		std::array<uint8_t, STACK_SIZE> stack = {};
		std::array<uint8_t, STACK_SIZE> variableStack = {};
		std::array<bytecodeOffset, CALL_STACK_SIZE> callStack = {};
		uint32_t stackPos = 0;
		uint32_t variableStackPos = 0;
		uint32_t callStackPos = 0;
		DebugInfo* debug = nullptr;
		BinaryBuffer* bytecodeBuffer = nullptr;
		std::vector<VTableEntry>* vTable = nullptr;
	};
} // namespace lang