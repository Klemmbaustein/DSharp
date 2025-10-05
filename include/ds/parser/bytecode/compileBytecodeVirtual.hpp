#pragma once
#include "compileBytecode.hpp"
#include <ds/parser/function.hpp>

namespace ds
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