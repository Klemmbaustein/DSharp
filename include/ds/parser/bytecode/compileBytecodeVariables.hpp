#pragma once
#include "compileBytecode.hpp"

namespace ds
{
	class Type;

	class BytecodePushVariable : public BytecodeInstruction
	{
	public:
		BytecodePushVariable(std::string name, Type* variableType, bool isInternal);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;
		void addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section, ds::DebugSection* debug) override;

		Type* variableType = nullptr;
		std::string name;
		uint32_t variablePos = 0;
		bool isInvalid = false;
		bool isInternal = false;

		void unrefHere(BytecodeBuffer* buffer) override;
	};

	class BytecodeReadVariable : public BytecodeInstruction
	{
	public:
		BytecodeReadVariable(BytecodePushVariable* variablePtr);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		BytecodePushVariable* variable = nullptr;
	};
	class BytecodeStoreVariable : public BytecodeInstruction
	{
	public:
		BytecodeStoreVariable(BytecodePushVariable* variablePtr);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		BytecodePushVariable* variable = nullptr;
	};
	class BytecodePopVariable : public BytecodeInstruction
	{
	public:
		BytecodePopVariable(uint32_t size, bool isScopeExit, bool isUnreachable);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		void addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section, ds::DebugSection* debug) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;

		uint32_t popSize = 0;
		bool isScopeExit = false;
		bool isUnreachable = false;
	};

} // namespace ds