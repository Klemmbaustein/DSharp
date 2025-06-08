#pragma once
#include "compileBytecode.hpp"
#include <parser/function.hpp>

namespace lang
{
	class BytecodeCallVirtual : public BytecodeInstruction
	{
	public:
		BytecodeCallVirtual(Function* fn, ClassType* languageClass);

		void getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler) override;
		bytecodeOffset getArgsSize() override;
		std::string toString() override;

		Function* functionToCall = nullptr;
		ClassType* languageClass = nullptr;
	};

}