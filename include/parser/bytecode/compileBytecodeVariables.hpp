#pragma once
#include "compileBytecode.hpp"

namespace lang
{
	class Type;

	class BytecodePushVariable : public BytecodeInstruction
	{
	public:
		BytecodePushVariable(std::string name, Type* variableType);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		Type* variableType = nullptr;
		std::string name;
		uint32_t variablePos = 0;
	};

	class BytecodeReadVariable : public BytecodeInstruction
	{
	public:
		BytecodeReadVariable(BytecodePushVariable* variablePtr);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		BytecodePushVariable* variable = nullptr;
	};
	class BytecodeStoreVariable : public BytecodeInstruction
	{
	public:
		BytecodeStoreVariable(BytecodePushVariable* variablePtr);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		BytecodePushVariable* variable = nullptr;
	};
	class BytecodePopVariable : public BytecodeInstruction
	{
	public:
		BytecodePopVariable(uint32_t size, bool isScopeExit);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		uint32_t popSize = 0;
		bool isScopeExit = false;
	};

} // namespace lang