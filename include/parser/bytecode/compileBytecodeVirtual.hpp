#pragma once
#include "compileBytecode.hpp"
#include <parser/function.hpp>

namespace lang
{
	class BytecodeCallVirtual : public BytecodeInstruction
	{
	public:
		BytecodeCallVirtual(Function* fn);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		Function* functionToCall = nullptr;
	};

}