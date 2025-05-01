#include "bytecode.hpp"

void lang::BytecodeStream::addOperation(BytecodeOp operationCode, const BinaryBuffer& argument)
{
	code.addValue(operationCode);
	code.addValue(uint8_t(argument.buffer.size()));
	code.addBuffer(argument);
}

void lang::BytecodeStream::addOperation(BytecodeOp operationCode)
{
	code.addValue(operationCode);
	code.addValue(uint8_t(0));
}
