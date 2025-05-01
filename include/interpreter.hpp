#pragma once
#include "bytecode.hpp"
#include <array>
#include <native/externalFunction.hpp>

namespace lang
{
	struct LanguageContext;

	struct InterpretContext
	{
		InterpretContext(LanguageContext* from);

		void loadBytecode(BytecodeStream* code);

		void run();

		template <typename T>
		T popValue()
		{
			stackPos -= sizeof(T);
			return *(T*)&this->stack[stackPos];
		}

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

	private:
		constexpr static size_t STACK_SIZE = 8000;
		constexpr static size_t FUNCTION_STACK_SIZE = 512;

		LanguageContext* language = nullptr;

		std::vector<ExternalFunctionPointer> externals;

		std::array<uint8_t, STACK_SIZE> stack = {};
		std::array<uint8_t, STACK_SIZE> variableStack = {};
		std::array<bytecodeOffset, FUNCTION_STACK_SIZE> functionStack = {};
		size_t stackPos = 0;
		size_t variableStackPos = 0;
		size_t functionStackPos = 0;
		BinaryBuffer* bytecodeBuffer = nullptr;
	};
} // namespace lang