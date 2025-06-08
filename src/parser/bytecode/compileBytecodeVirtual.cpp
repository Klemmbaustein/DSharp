#include <parser/bytecode/compileBytecodeVirtual.hpp>
#include <parser/types/classType.hpp>
#include <parser/parser.hpp>
#include <format>

using namespace lang;

lang::BytecodeCallVirtual::BytecodeCallVirtual(Function* fn, ClassType* languageClass)
{
	this->functionToCall = fn;
	this->languageClass = languageClass;
	this->operation = BytecodeOp::virtualCall;
}

void lang::BytecodeCallVirtual::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue(static_cast<ParsedFunction*>(this->functionToCall)->vTableOffset);
}

bytecodeOffset lang::BytecodeCallVirtual::getArgsSize()
{
	return sizeof(bytecodeOffset);
}

std::string lang::BytecodeCallVirtual::toString()
{
	return std::format("\tVIRTUAL_CALL {}", this->functionToCall->getFullName());
}
