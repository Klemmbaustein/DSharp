#pragma once
#include "compileBytecode.hpp"

namespace ds
{
	class Type;

	class BytecodePushVariable : public BytecodeInstruction
	{
	public:
		BytecodePushVariable(std::string name, Type* variableType);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;
		void addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section) override;

		Type* variableType = nullptr;
		std::string name;
		uint32_t variablePos = 0;
	};

	class BytecodeReadVariable : public BytecodeInstruction
	{
	public:
		BytecodeReadVariable(std::shared_ptr<BytecodePushVariable> variablePtr);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		std::shared_ptr<BytecodePushVariable> variable = nullptr;
	};
	class BytecodeStoreVariable : public BytecodeInstruction
	{
	public:
		BytecodeStoreVariable(std::shared_ptr<BytecodePushVariable> variablePtr);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		std::shared_ptr<BytecodePushVariable> variable = nullptr;
	};
	class BytecodePopVariable : public BytecodeInstruction
	{
	public:
		BytecodePopVariable(uint32_t size, bool isScopeExit);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		void addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		uint32_t popSize = 0;
		bool isScopeExit = false;
	};

} // namespace ds