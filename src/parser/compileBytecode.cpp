#include <parser/compileBytecode.hpp>
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

std::string BytecodeOperation::toString()
{
	static std::map<BytecodeOp, const char*> operations = {
		{ BytecodeOp::push, "PUSH" },
		{ BytecodeOp::pop, "POP" },
		{ BytecodeOp::call, "CALL" },
		{ BytecodeOp::jump, "JUMP" },
		{ BytecodeOp::callExternal, "CALL_EXTERNAL" },
		{ BytecodeOp::ret, "RETURN" },
		{ BytecodeOp::addInt, "ADD_I32" },
		{ BytecodeOp::subInt, "SUB_I32" },
		{ BytecodeOp::mulInt, "MUL_I32" },
		{ BytecodeOp::divInt, "DIV_I32" },
		{ BytecodeOp::addFloat, "ADD_F32" },
		{ BytecodeOp::subFloat, "SUB_F32" },
		{ BytecodeOp::mulFloat, "MUL_F32" },
		{ BytecodeOp::divFloat, "DIV_F32" },
		{ BytecodeOp::intToFloat, "INT_TO_FLOAT" },
		{ BytecodeOp::floatToInt, "FLOAT_TO_INT" },
		{ BytecodeOp::pushVariable, "PUSH_VAR" },
		{ BytecodeOp::storeVariable, "STORE_VAR" },
		{ BytecodeOp::readVariable, "READ_VAR" },
		{ BytecodeOp::popVariable, "POP_VAR" },
		{ BytecodeOp::allocClass, "NEW_CLASS" },
		{ BytecodeOp::unrefClass, "UNREF_CLASS" },
		{ BytecodeOp::classMember, "CLASS_MEMBER" },
		{ BytecodeOp::setClassMember, "SET_CLASS_MEMBER" },
	};

	std::string op = operations[this->operation];

	if (op.empty())
		op = std::to_string(int(this->operation));

	return std::format("\t{} {}", op, this->arguments.toString());
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
	this->function = function;
	this->operation = BytecodeOp::callExternal;
}

std::string BytecodeCallNative::toString()
{
	return std::format("\tNATIVE_CALL {}", this->function->getFullName());
}

void lang::BytecodeCallNative::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	std::string fullName = this->function->getFullName();
	auto foundPosition = compiler->usedExternals.find(fullName);

	uint32_t position = 0;

	if (foundPosition == compiler->usedExternals.end())
	{
		position = uint32_t(compiler->externals.size());
		compiler->externals.push_back(fullName);
		compiler->usedExternals.insert({ fullName, position });
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

lang::BytecodeJump::BytecodeJump(BytecodeJumpLabel* target)
{
	this->operation = BytecodeOp::jump;
	this->target = target;
}

void lang::BytecodeJump::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue(this->target->offset);
}

bytecodeOffset lang::BytecodeJump::getArgsSize()
{
	return sizeof(bytecodeOffset);
}

std::string BytecodeJump::toString()
{
	return std::format("\tJUMP ", this->target->name);
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

void lang::BytecodeCompiler::compileTo(BytecodeStream& stream)
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
			bytecodePos +=
				/* operation */ sizeof(BytecodeOp) + /* arguments size */ sizeof(uint8_t) +
				/* arguments */ +instr->getArgsSize();
		}
	}

	for (auto& code : orderedFunctions)
	{
		for (BytecodeInstruction* instr : code->instructions)
		{
			if (instr->baseSize != 0)
			{
				BinaryBuffer argsBuffer;
				instr->getArgs(argsBuffer, this);
				stream.addOperation(instr->operation, argsBuffer);
			}
		}
	}

	stream.externalFunctions = this->externals;
}

void lang::BytecodeBuffer::add(BytecodeInstruction* instruction)
{
	this->instructions.push_back(instruction);
}

void lang::BytecodeBuffer::addOperation(BytecodeOp operation, const BinaryBuffer& arguments)
{
	add(new BytecodeOperation(operation, arguments));
}

void lang::BytecodeBuffer::addOperation(BytecodeOp operation)
{
	add(new BytecodeOperation(operation, BinaryBuffer()));
}

void lang::BytecodeBuffer::addBuffer(const BytecodeBuffer& other)
{
	this->instructions.reserve(this->instructions.size() + other.instructions.size());

	for (auto& instr : other.instructions)
	{
		this->instructions.push_back(instr);
	}
}
