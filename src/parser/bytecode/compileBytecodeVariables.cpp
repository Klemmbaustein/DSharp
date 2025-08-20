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
#if HAS_CPP_FORMAT
	return std::format("\tPUSH_VAR {} {} {}", this->variableType->name, this->name,
		this->variableType->size);
#else
	return "\tPUSH_VAR";
#endif
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

lang::BytecodeReadVariable::BytecodeReadVariable(std::shared_ptr<BytecodePushVariable> variablePtr)
{
	this->variable = variablePtr;
	this->operation = BytecodeOp::readVariable;
}

std::string BytecodeReadVariable::toString()
{

#if HAS_CPP_FORMAT
	return std::format("\tREAD_VAR {} {} <pos {}>", this->variable->variableType->name,
		this->variable->name, this->variable->variablePos);
#else
	return "\tREAD_VAR";
#endif
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

lang::BytecodeStoreVariable::BytecodeStoreVariable(std::shared_ptr<BytecodePushVariable> variablePtr)
{
	this->variable = variablePtr;
	this->operation = BytecodeOp::storeVariable;
}

std::string BytecodeStoreVariable::toString()
{
#if HAS_CPP_FORMAT
	return std::format("\tSTORE_VAR {} {}", this->variable->variableType->name,
		this->variable->name);
#else
	return "\tSTORE_VAR";
#endif
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

lang::BytecodePopVariable::BytecodePopVariable(uint32_t size, bool isScopeExit)
{
	this->operation = BytecodeOp::popVariable;
	this->isScopeExit = isScopeExit;
	popSize = size;
}

std::string BytecodePopVariable::toString()
{
#if HAS_CPP_FORMAT
	return std::format("\tPOP_VAR {}", this->popSize);
#else
	return "\tPOP_VAR";
#endif
}
void BytecodePopVariable::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	if (!this->isScopeExit)
	{
		compiler->variableStackPosition -= this->popSize;
	}
	stream.addValue<uint32_t>(this->popSize);
}

bytecodeOffset BytecodePopVariable::getArgsSize()
{
	return sizeof(uint32_t);
}
