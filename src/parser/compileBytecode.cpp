#include <parser/compileBytecode.hpp>
#include <parser/classType.hpp>
#include <native/nativeModule.hpp>
#include <print>
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
		{ BytecodeOp::lessInt, "LESS_I32" },
		{ BytecodeOp::equals, "EQUALS" },
		{ BytecodeOp::lessInt, "LESS_I32" },
		{ BytecodeOp::addFloat, "ADD_F32" },
		{ BytecodeOp::subFloat, "SUB_F32" },
		{ BytecodeOp::mulFloat, "MUL_F32" },
		{ BytecodeOp::divFloat, "DIV_F32" },
		{ BytecodeOp::intToFloat, "INT_TO_FLOAT" },
		{ BytecodeOp::floatToInt, "FLOAT_TO_INT" },
		{ BytecodeOp::boolAnd, "BOOL_AND" },
		{ BytecodeOp::boolOr, "BOOL_OR" },
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
	};

	std::string op = operations[this->operation];

	if (op.empty())
		op = std::to_string(int(this->operation));

	return std::format("\t{} {}", op, arguments.toString());
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
	return std::format("\tCALL {}", this->callName);
}

void lang::BytecodeCallFunction::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue(compiler->functions[this->callName].offset);
}

bytecodeOffset lang::BytecodeCallFunction::getArgsSize()
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
	return std::format("\tNATIVE_CALL {}", functionName);
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

	std::println("{}: {}", this->functionName, position);

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
	return std::format("{}:", this->name);
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
	stream.addValue(uint32_t(languageClass->destructor ? compiler->functions[languageClass->destructor->getFullName()].offset : 0));
}

bytecodeOffset lang::BytecodeAllocClass::getArgsSize()
{
	return sizeof(uint32_t) * 2;
}

std::string BytecodeAllocClass::toString()
{
	return std::format("\tALLOC {}", this->languageClass->name);
}

void lang::BytecodeCompiler::printAssembly()
{
	for (auto& fn : this->functions)
	{
		std::println("FUNCTION: {} (offset 0x{:x})", fn.first, fn.second.offset);

		for (auto& instr : fn.second.instructions)
		{
			std::println("{}", instr->toString());
		}
	}
}

void lang::BytecodeCompiler::compileTo(BytecodeStream& stream, ErrorContext* errors)
{
	bytecodeOffset bytecodePos = 0;

	std::vector<BytecodeFunction*> orderedFunctions;

	for (auto& i : this->functions)
	{
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
		for (BytecodeInstruction* instr : code->instructions)
		{
			instr->offset = bytecodePos;
			if (instr->baseSize > 0)
			{
				bytecodePos +=
					/* operation */ sizeof(BytecodeOp) + /* arguments size */ sizeof(uint8_t) +
					/* arguments */ +instr->getArgsSize();
			}
		}
	}

	BinaryBuffer argsBuffer;
	for (auto& code : orderedFunctions)
	{
		for (BytecodeInstruction* instr : code->instructions)
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

		if (this->variableStackPosition != 0)
		{
			errors->error(ErrorCode::internalError, Token(),
				"Internal compiler error. The variable stack position did not return to 0 after a function call. "
				"This is a compiler bug and should never happen.");
		}
	}

	stream.externalFunctions = this->externals;
}

void lang::BytecodeBuffer::add(BytecodeInstruction* instruction)
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
	add(new BytecodeOperation(operation, arguments));
}

void lang::BytecodeBuffer::addOperation(BytecodeOp operation)
{
	add(new BytecodeOperation(operation, BinaryBuffer()));
}

void lang::BytecodeBuffer::prependBuffer(const BytecodeBuffer& other)
{
	this->instructions.reserve(this->instructions.size() + other.instructions.size());

#ifdef _WIN32
	// GCC doesnt have thise function yet, cool
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
