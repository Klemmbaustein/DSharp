#include <ds/parser/bytecode/compileBytecodeDebug.hpp>

using namespace ds;

void ds::BytecodeDebugLine::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
}

BytecodeOffset ds::BytecodeDebugLine::getArgsSize()
{
	return BytecodeOffset();
}

ds::BytecodeDebugLine::BytecodeDebugLine(uint32_t line)
{
	this->line = line;
	this->baseSize = 0;
}

void ds::BytecodeDebugLine::addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section, ds::DebugSection* debug)
{
	if (debug)
	{
		debug->lines.push_back(DebugLine{
			.lineNumber = line,
			.offset = this->offset,
			});
	}
}

std::string ds::BytecodeDebugLine::toString()
{
	return "";
}
