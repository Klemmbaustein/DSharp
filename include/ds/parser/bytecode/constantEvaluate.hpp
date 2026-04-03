#pragma once
#include <ds/parser/bytecode/compileBytecode.hpp>

namespace ds
{
	class ConstantEvaluate
	{
	public:

		bool run(const BytecodeBuffer& buffer);

		template <typename T>
		T popValue()
		{
			stackPos -= sizeof(T);
			return *(T*)&this->stack[stackPos];
		}

		template <typename T>
		void pushValue(const T& value)
		{
			this->stack.resize(stackPos + sizeof(value));
			*((T*)&this->stack[stackPos]) = value;
			stackPos += sizeof(value);
		}

		void pushBytes(const uint8_t* data, Size size)
		{
			if (size == 0)
				return;

			this->stack.resize(stackPos + sizeof(size));
			memcpy(&this->stack[stackPos], data, size);
			stackPos += size;
		}
		void pushBytes(const BinaryBuffer& buffer)
		{
			if (buffer.buffer.size() == 0)
				return;

			this->stack.resize(stackPos + buffer.buffer.size());
			memcpy(&this->stack[stackPos], buffer.buffer.data(), buffer.buffer.size());
			stackPos += buffer.buffer.size();
		}

		void popBytes(uint8_t* to, Size size)
		{
			if (size == 0)
				return;

			stackPos -= size;
			memcpy(to, &this->stack[stackPos], size);
		}
		void copyBytes(Size size)
		{
			if (size == 0)
				return;

			memcpy(&this->stack[stackPos], &this->stack[stackPos - size], size);
			stackPos += size;
		}

	private:
		size_t stackPos = 0;
		std::vector<uint8_t> stack;
	};
} // namespace ds