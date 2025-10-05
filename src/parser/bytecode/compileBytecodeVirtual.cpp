#include <ds/parser/bytecode/compileBytecodeVirtual.hpp>
#include <ds/parser/types/classType.hpp>
#include <ds/parser/parser.hpp>
#include <format>

using namespace ds;

ds::BytecodeCallVirtual::BytecodeCallVirtual(Function* fn)
{
	this->functionToCall = fn;
	this->operation = BytecodeOp::virtualCall;
}

void ds::BytecodeCallVirtual::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue(this->functionToCall->getVirtualOffset());
}

bytecodeOffset ds::BytecodeCallVirtual::getArgsSize()
{
	return sizeof(bytecodeOffset);
}

std::string ds::BytecodeCallVirtual::toString()
{
	return "\tVIRTUAL_CALL " + this->functionToCall->getFullName();
}
