#include <ds/parser/bytecode/compileBytecodeUnwind.hpp>
#include <ds/parser/types/type.hpp>

using namespace ds;

ds::BytecodeUnwindClass::BytecodeUnwindClass(BytecodePushVariable* variable)
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

void ds::BytecodeUnwindClass::addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section,
	ds::DebugSection* debug)
{
	if (!variable->isInternal && debug)
	{
		debug->variables.push_back(DebugVariableInfo{
			.type = this->variable->variableType->id,
			.name = this->variable->name,
		});
	}

	section.parts.push_back(UnwindPart{
		.op = UnwindOp::popClass,
		.size = uint16_t(compiler->variableStackPosition - this->variable->variablePos),
		.debugId = !variable->isInternal && debug ? uint32_t(debug->variables.size() - 1) : UINT32_MAX,
		.start = this->variable->offset,
		.offset = this->offset,
	});
}

ds::BytecodeDebugUnwindPrimitive::BytecodeDebugUnwindPrimitive(BytecodePushVariable* variable)
{
	this->baseSize = 0;
	this->variable = variable;
}

void ds::BytecodeDebugUnwindPrimitive::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
}

BytecodeOffset ds::BytecodeDebugUnwindPrimitive::getArgsSize()
{
	return BytecodeOffset();
}

std::string ds::BytecodeDebugUnwindPrimitive::toString()
{
	return "UNWIND_PRIM";
}

void ds::BytecodeDebugUnwindPrimitive::addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section,
	ds::DebugSection* debug)
{
	if (variable->isInternal && !debug)
	{
		return;
	}

	debug->variables.push_back(DebugVariableInfo{
		.type = this->variable->variableType->id,
		.name = this->variable->name,
	});

	section.parts.push_back(UnwindPart{
		.op = UnwindOp::debugPopPrimitive,
		.size = uint16_t(compiler->variableStackPosition - this->variable->variablePos),
		.debugId = !variable->isInternal ? uint32_t(debug->variables.size() - 1) : UINT32_MAX,
		.start = this->variable->offset,
		.offset = this->offset,
	});
}
