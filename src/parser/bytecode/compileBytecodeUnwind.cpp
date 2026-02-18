#include <ds/parser/bytecode/compileBytecodeUnwind.hpp>

using namespace ds;

ds::BytecodeUnwindClass::BytecodeUnwindClass(std::shared_ptr<BytecodePushVariable> variable)
{
	this->baseSize = 0;
	this->variable = variable;
}

void ds::BytecodeUnwindClass::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
}

BytecodeOffset ds::BytecodeUnwindClass::getArgsSize()
{
	return BytecodeOffset();
}

std::string ds::BytecodeUnwindClass::toString()
{
	return "UNWIND: " + variable->toString();
}

void ds::BytecodeUnwindClass::addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section)
{
	section.parts.push_back(UnwindPart{
		.op = UnwindOp::popClass,
		.size = uint8_t(compiler->variableStackPosition - this->variable->variablePos),
		.start = this->variable->offset,
		.offset = this->offset,
	});
}
