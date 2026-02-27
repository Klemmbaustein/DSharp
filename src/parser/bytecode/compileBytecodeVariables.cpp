#include <ds/parser/bytecode/compileBytecodeVariables.hpp>
#include <ds/parser/types/type.hpp>
#include <format>

using namespace ds;

ds::BytecodePushVariable::BytecodePushVariable(std::string name, Type* variableType)
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

void ds::BytecodePushVariable::addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section)
{
	section.parts.push_back(UnwindPart{
		.op = UnwindOp::pushBytes,
		.size = uint8_t(this->variableType->size),
		.offset = this->offset,
	});
}

void BytecodePushVariable::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	this->variablePos = compiler->variableStackPosition;
	compiler->variableStackPosition += this->variableType->size;
	stream.addValue<uint32_t>(this->variableType->size);
}

BytecodeOffset BytecodePushVariable::getArgsSize()
{
	return sizeof(uint32_t);
}

ds::BytecodeReadVariable::BytecodeReadVariable(BytecodePushVariable* variablePtr)
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

BytecodeOffset BytecodeReadVariable::getArgsSize()
{
	return sizeof(uint32_t) * 2;
}

ds::BytecodeStoreVariable::BytecodeStoreVariable(BytecodePushVariable* variablePtr)
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

BytecodeOffset BytecodeStoreVariable::getArgsSize()
{
	return sizeof(uint32_t) * 2;
}

ds::BytecodePopVariable::BytecodePopVariable(uint32_t size, bool isScopeExit, bool isUnreachable)
{
	this->operation = BytecodeOp::popVariable;
	this->isScopeExit = isScopeExit;
	this->isUnreachable = isUnreachable;
	popSize = size;
	if (isUnreachable)
	{
		this->baseSize = 0;
	}
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
	if (isUnreachable)
	{
		return;
	}
	stream.addValue<Size>(this->popSize);
}

void ds::BytecodePopVariable::addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section)
{
	section.parts.push_back(UnwindPart{
		.op = UnwindOp::popBytes,
		.size = uint8_t(this->popSize),
		.offset = this->offset,
		});
}

BytecodeOffset BytecodePopVariable::getArgsSize()
{
	return this->isUnreachable ? 0 : sizeof(Size);
}
