#include <parser/bytecode/compileBytecodeVariables.hpp>
#include <parser/types/type.hpp>
#include <format>

using namespace lang;

lang::BytecodePushVariable::BytecodePushVariable(std::string name, Type* variableType)
{
	this->name = name;
	this->variableType = variableType;
	this->operation = BytecodeOp::pushVariable;
}

std::string BytecodePushVariable::toString()
{
	return std::format("\tPUSH_VAR {} {}", this->variableType->name, this->name);
}
void BytecodePushVariable::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	this->variablePos = compiler->variableStackPosition;
	compiler->variableStackPosition += this->variableType->size;
	stream.addValue<uint32_t>(this->variableType->size);
}

bytecodeOffset BytecodePushVariable::getArgsSize()
{
	return sizeof(uint32_t);
}

lang::BytecodeReadVariable::BytecodeReadVariable(BytecodePushVariable* variablePtr)
{
	this->variable = variablePtr;
	this->operation = BytecodeOp::readVariable;
}

std::string BytecodeReadVariable::toString()
{
	return std::format("\tREAD_VAR {} {} <pos {}>", this->variable->variableType->name, this->variable->name, this->variable->variablePos);
}
void BytecodeReadVariable::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue<uint32_t>(this->variable->variableType->size);
	stream.addValue<uint32_t>(compiler->variableStackPosition - this->variable->variablePos);
}

bytecodeOffset BytecodeReadVariable::getArgsSize()
{
	return sizeof(uint32_t) * 2;
}

lang::BytecodeStoreVariable::BytecodeStoreVariable(BytecodePushVariable* variablePtr)
{
	this->variable = variablePtr;
	this->operation = BytecodeOp::storeVariable;
}

std::string BytecodeStoreVariable::toString()
{
	return std::format("\tSTORE_VAR {} {}", this->variable->variableType->name, this->variable->name);
}
void BytecodeStoreVariable::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue<uint32_t>(this->variable->variableType->size);
	stream.addValue<uint32_t>(compiler->variableStackPosition - this->variable->variablePos);
}

bytecodeOffset BytecodeStoreVariable::getArgsSize()
{
	return sizeof(uint32_t) * 2;
}

lang::BytecodePopVariable::BytecodePopVariable(uint32_t size)
{
	this->operation = BytecodeOp::popVariable;
	popSize = size;
}

std::string BytecodePopVariable::toString()
{
	return std::format("\tPOP_VAR {}", this->popSize);
}
void BytecodePopVariable::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	compiler->variableStackPosition -= this->popSize;
	stream.addValue<uint32_t>(this->popSize);
}

bytecodeOffset BytecodePopVariable::getArgsSize()
{
	return sizeof(uint32_t);
}
