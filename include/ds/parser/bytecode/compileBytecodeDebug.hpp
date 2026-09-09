#pragma once
#include <ds/parser/bytecode/compileBytecode.hpp>
#include <cstdint>

namespace ds
{
	class BytecodeDebugLine : public BytecodeInstruction
	{
	public:
		BytecodeDebugLine(uint32_t line);

		uint32_t line = 0;

		// Inherited via BytecodeInstruction
		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		BytecodeOffset getArgsSize() override;
		void addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section, ds::DebugSection* debug) override;
		std::string toString() override;
	};
}