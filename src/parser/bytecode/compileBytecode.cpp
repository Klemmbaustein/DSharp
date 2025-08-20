#include <parser/bytecode/compileBytecode.hpp>
#include <parser/types/classType.hpp>
#include <native/nativeModule.hpp>
#include <print>
#include <stdlib.h>
using namespace lang;

// ------------- //
// Operation     //
// ------------- //

lang::BytecodeOperation::BytecodeOperation(BytecodeOp operation, BinaryBuffer arguments)
{
	this->operation = operation;
	this->arguments = arguments;
}
void lang::BytecodeOperation::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addBuffer(this->arguments);
}
bytecodeOffset lang::BytecodeOperation::getArgsSize()
{
	return bytecodeOffset(this->arguments.buffer.size());
}

std::string lang::BytecodeInstruction::toStringDefault(const BinaryBuffer& arguments)
{
	static std::map<BytecodeOp, const char*> operations = {
		{ BytecodeOp::push, "PUSH" },
		{ BytecodeOp::pop, "POP" },
		{ BytecodeOp::call, "CALL" },
		{ BytecodeOp::jump, "JUMP" },
		{ BytecodeOp::jumpIfNot, "JUMP_IF_NOT" },
		{ BytecodeOp::callExternal, "CALL_EXTERNAL" },
		{ BytecodeOp::ret, "RETURN" },
		{ BytecodeOp::copy, "COPY" },
		{ BytecodeOp::addInt, "ADD_I32" },
		{ BytecodeOp::subInt, "SUB_I32" },
		{ BytecodeOp::mulInt, "MUL_I32" },
		{ BytecodeOp::divInt, "DIV_I32" },
		{ BytecodeOp::greaterInt, "GREATER_I32" },
		{ BytecodeOp::equals, "EQUALS" },
		{ BytecodeOp::addFloat, "ADD_F32" },
		{ BytecodeOp::subFloat, "SUB_F32" },
		{ BytecodeOp::mulFloat, "MUL_F32" },
		{ BytecodeOp::divFloat, "DIV_F32" },
		{ BytecodeOp::greaterInt, "GREATER_F32" },
		{ BytecodeOp::intToFloat, "INT_TO_FLOAT" },
		{ BytecodeOp::floatToInt, "FLOAT_TO_INT" },
		{ BytecodeOp::boolAnd, "BOOL_AND" },
		{ BytecodeOp::boolOr, "BOOL_OR" },
		{ BytecodeOp::boolNot, "BOOL_NOT" },
		{ BytecodeOp::pushVariable, "PUSH_VAR" },
		{ BytecodeOp::storeVariable, "STORE_VAR" },
		{ BytecodeOp::readVariable, "READ_VAR" },
		{ BytecodeOp::popVariable, "POP_VAR" },
		{ BytecodeOp::allocClass, "NEW_CLASS" },
		{ BytecodeOp::refClass, "REF_CLASS" },
		{ BytecodeOp::unrefClass, "UNREF_CLASS" },
		{ BytecodeOp::classMember, "CLASS_MEMBER" },
		{ BytecodeOp::setClassMember, "SET_CLASS_MEMBER" },
		{ BytecodeOp::setClassMemberPushAgain, "SET_CLASS_MEMBER_PUSH" },
		{ BytecodeOp::concatString, "CONCAT_STRING" },
		{ BytecodeOp::indexString, "INDEX_STRING" },
		{ BytecodeOp::setStringIndexCopy, "SET_STR_AT" },
		{ BytecodeOp::nullCheck, "CHECK_NOT_NULL" },
	};

	const char* op = operations[this->operation];
	std::string opString;
	if (!op)
		opString = std::to_string(int(this->operation));
	else
		opString = op;

	return "\t" + opString + " " + arguments.toString();
}

std::string BytecodeOperation::toString()
{
	return toStringDefault(this->arguments);
}

// ------------- //
// CallFunction  //
// ------------- //

lang::BytecodeCallFunction::BytecodeCallFunction(std::string callName)
{
	this->operation = BytecodeOp::call;
	this->callName = callName;
}

std::string BytecodeCallFunction::toString()
{
	return "\tCALL " + this->callName;
}

void lang::BytecodeCallFunction::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue(compiler->functions[this->callName].offset);
}

bytecodeOffset lang::BytecodeCallFunction::getArgsSize()
{
	return sizeof(bytecodeOffset);
}

// --------------- //
// FunctionAddress //
// --------------- //

lang::BytecodeFunctionAddress::BytecodeFunctionAddress(std::string callName, bool native)
{
	this->operation = BytecodeOp::push;
	this->callName = callName;
	this->native = native;
}

std::string BytecodeFunctionAddress::toString()
{
	return "\tPUSH ADDROFF {}" + this->callName;
}

void lang::BytecodeFunctionAddress::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	if (native)
	{
		auto foundPosition = compiler->usedExternals.find(this->callName);

		uint32_t position = 0;

		if (foundPosition == compiler->usedExternals.end())
		{
			position = uint32_t(compiler->externals.size());
			compiler->externals.push_back(this->callName);
			compiler->usedExternals.insert({ this->callName, position });
		}
		else
		{
			position = foundPosition->second;
		}

		stream.addValue(position);
	}
	else
	{
		stream.addValue(compiler->functions[this->callName].offset);
	}
}

bytecodeOffset lang::BytecodeFunctionAddress::getArgsSize()
{
	return sizeof(bytecodeOffset);
}

// ------------- //
// CallNative    //
// ------------- //

lang::BytecodeCallNative::BytecodeCallNative(NativeFunction* function)
{
	this->functionName = function->getFullName();
	this->operation = BytecodeOp::callExternal;
}

lang::BytecodeCallNative::BytecodeCallNative(std::string functionName)
{
	this->operation = BytecodeOp::callExternal;
	this->functionName = functionName;
}

std::string BytecodeCallNative::toString()
{
	return "\tNATIVE_CALL " + functionName;
}

void lang::BytecodeCallNative::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	auto foundPosition = compiler->usedExternals.find(this->functionName);

	uint32_t position = 0;

	if (foundPosition == compiler->usedExternals.end())
	{
		position = uint32_t(compiler->externals.size());
		compiler->externals.push_back(this->functionName);
		compiler->usedExternals.insert({ this->functionName, position });
	}
	else
	{
		position = foundPosition->second;
	}

	stream.addValue(position);
}

bytecodeOffset lang::BytecodeCallNative::getArgsSize()
{
	return sizeof(uint32_t);
}

// ------------- //
// JumpLabel     //
// ------------- //

lang::BytecodeJumpLabel::BytecodeJumpLabel(std::string name)
{
	this->name = name;
	this->baseSize = 0;
}

void lang::BytecodeJumpLabel::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
}

bytecodeOffset lang::BytecodeJumpLabel::getArgsSize()
{
	return 0;
}
std::string BytecodeJumpLabel::toString()
{
	return this->name + ":";
}

// ------------- //
// Jump          //
// ------------- //

lang::BytecodeJump::BytecodeJump(BytecodeOp operation, BytecodeJumpLabel* target)
{
	this->operation = operation;
	this->target = target;
}

void lang::BytecodeJump::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue<bytecodeOffset>(this->target->offset);
}

bytecodeOffset lang::BytecodeJump::getArgsSize()
{
	return sizeof(bytecodeOffset);
}

std::string BytecodeJump::toString()
{
	return this->toStringDefault(BinaryBuffer()) + "-> " + this->target->name;
}

// ------------- //
// AllocClass    //
// ------------- //

lang::BytecodeAllocClass::BytecodeAllocClass(ClassType* languageClass)
{
	this->languageClass = languageClass;
	this->operation = BytecodeOp::allocClass;
}
void lang::BytecodeAllocClass::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	// typeid
	stream.addValue(uint32_t(0));
	// vtable
	stream.addValue(languageClass->vTableOffset);
}

bytecodeOffset lang::BytecodeAllocClass::getArgsSize()
{
	return sizeof(uint32_t) * 2;
}

std::string BytecodeAllocClass::toString()
{
	return "\tALLOC " + this->languageClass->name;
}

void lang::BytecodeCompiler::printAssembly()
{
	for (auto& fn : this->functions)
	{
#if HAS_CPP_FORMAT
		std::println("FUNCTION: {} (offset 0x{:x})", fn.first, fn.second.offset);
#else
		std::printf("FUNCTION: %s (offset 0x%lx)", fn.first.c_str(), fn.second.offset);
#endif

		for (auto& instr : fn.second.instructions)
		{
#if HAS_CPP_FORMAT
			std::println("{} {}", instr->offset, instr->toString());
#else
			std::printf("%ul %s", instr->offset, instr->toString().c_str());
#endif
		}
	}
}

void lang::BytecodeCompiler::compileTo(BytecodeStream& stream, std::vector<Function*> virtualTable, ErrorContext* errors)
{
	bytecodeOffset bytecodePos = 0;

	std::vector<BytecodeFunction*> orderedFunctions;

	for (auto& i : this->functions)
	{
		i.second.name = i.first;
		if (i.second.isEntryPoint)
		{
			orderedFunctions.insert(orderedFunctions.begin(), &i.second);
		}
		else
		{
			orderedFunctions.push_back(&i.second);
		}
	}

	// First get all offsets so jump operations will know the proper offset.
	for (auto& code : orderedFunctions)
	{
		code->offset = bytecodePos;
		for (auto& instr : code->instructions)
		{
			instr->offset = bytecodePos;
			if (instr->baseSize > 0)
			{
				bytecodePos +=
					/* operation */ sizeof(BytecodeOp) + /* arguments size */ sizeof(uint8_t) +
					/* arguments */ instr->getArgsSize();
			}
		}
	}

	for (auto& i : virtualTable)
	{
		if (i)
		{
			stream.virtualTable.push_back(VTableEntry{
				.codeOffset = functions[i->getFullName()].offset });
		}
		else
		{
			stream.virtualTable.push_back(VTableEntry());
		}
	}

	BinaryBuffer argsBuffer;
	for (auto& code : orderedFunctions)
	{
		stream.debug.sections.push_back(DebugSection{
			.offset = code->offset,
			.name = code->name,
		});

		for (auto& instr : code->instructions)
		{
			if (instr->baseSize != 0)
			{
				argsBuffer.clear();
				instr->getArgs(argsBuffer, this);
				if (argsBuffer.buffer.size() != instr->getArgsSize())
				{
					abort();
				}
				stream.addOperation(instr->operation, argsBuffer);
			}
		}

		this->variableStackPosition = 0;
	}

	stream.externalFunctions = this->externals;
}

void lang::BytecodeBuffer::add(InstructionPtr instruction)
{
	this->instructions.push_back(instruction);
}

void lang::BytecodeBuffer::pushInt(uint32_t data)
{
	static BinaryBuffer args;
	args.clear();
	args.addValue(data);
	addOperation(BytecodeOp::push, args);
}

void lang::BytecodeBuffer::addOperation(BytecodeOp operation, const BinaryBuffer& arguments)
{
	add(std::make_shared<BytecodeOperation>(operation, arguments));
}

void lang::BytecodeBuffer::addOperation(BytecodeOp operation)
{
	add(std::make_shared<BytecodeOperation>(operation, BinaryBuffer()));
}

void lang::BytecodeBuffer::prependBuffer(const BytecodeBuffer& other)
{
	this->instructions.reserve(this->instructions.size() + other.instructions.size());

#ifdef _WIN32
	// GCC doesn't have this function yet, cool
	this->instructions.insert_range(instructions.begin(), other.instructions);
#else
//	this->instructions.insert(instructions.begin(), other.instructions);
#endif
}
void lang::BytecodeBuffer::addBuffer(const BytecodeBuffer& other)
{
	this->instructions.reserve(this->instructions.size() + other.instructions.size());

	for (auto& instr : other.instructions)
	{
		this->instructions.push_back(instr);
	}
}
