#pragma once
#include "compileBytecode.hpp"
#include "compileBytecodeVariables.hpp"

namespace ds
{
	class Type;

	class BytecodeUnwindClass : public BytecodeInstruction
	{
	public:
		BytecodeUnwindClass(std::shared_ptr<BytecodePushVariable> variable);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;
		void addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section) override;

		std::shared_ptr<BytecodePushVariable> variable;
	};

} // namespace ds