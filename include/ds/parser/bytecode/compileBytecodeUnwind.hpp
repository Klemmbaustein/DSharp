#pragma once
#include "compileBytecode.hpp"
#include "compileBytecodeVariables.hpp"

namespace ds
{
	class Type;

	class BytecodeUnwindClass : public BytecodeInstruction
	{
	public:
		BytecodeUnwindClass(BytecodePushVariable* variable);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;
		void addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section, ds::DebugSection* debug) override;

		BytecodePushVariable* variable;
	};

	class BytecodeDebugUnwindPrimitive : public BytecodeInstruction
	{
	public:
		BytecodeDebugUnwindPrimitive(BytecodePushVariable* variable);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		std::string toString() override;
		void addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section, ds::DebugSection* debug) override;

		BytecodePushVariable* variable;
	};


} // namespace ds