#include <parser/bytecode/compileBytecodeVirtual.hpp>
#include <parser/types/classType.hpp>
#include <parser/parser.hpp>
#include <format>

using namespace lang;

lang::BytecodeCallVirtual::BytecodeCallVirtual(Function* fn)
{
	this->functionToCall = fn;
	this->operation = BytecodeOp::virtualCall;
}

void lang::BytecodeCallVirtual::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue(this->functionToCall->getVirtualOffset());
}

bytecodeOffset lang::BytecodeCallVirtual::getArgsSize()
{
	return sizeof(bytecodeOffset);
}

std::string lang::BytecodeCallVirtual::toString()
{
	return "\tVIRTUAL_CALL " + this->functionToCall->getFullName();
}
