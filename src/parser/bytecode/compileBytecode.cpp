#include <ds/parser/bytecode/compileBytecode.hpp>
#include <ds/parser/types/classType.hpp>
#include <ds/native/nativeModule.hpp>
#include <print>
#include <stdlib.h>
using namespace ds;

// ------------- //
// Operation     //
// ------------- //

ds::BytecodeOperation::BytecodeOperation(BytecodeOp operation, BinaryBuffer arguments)
{
	this->operation = operation;
	this->arguments = arguments;
}
void ds::BytecodeOperation::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addBuffer(this->arguments);
}
bytecodeOffset ds::BytecodeOperation::getArgsSize()
{
	return bytecodeOffset(this->arguments.buffer.size());
}

void ds::BytecodeInstruction::addUnwindInfo(BytecodeCompiler* compiler, UnwindSection& section)
{
}

std::string ds::BytecodeInstruction::toStringDefault(const BinaryBuffer& arguments) const
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
		{ BytecodeOp::getStructMember, "GET_STRUCT_MEMBER" },
		{ BytecodeOp::setStructMember, "SET_STRUCT_MEMBER" },
		{ BytecodeOp::suspend, "SUSPEND" },
		{ BytecodeOp::awaitTask, "AWAIT_TASK" },
		{ BytecodeOp::unwind, "UNWIND" },
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

ds::BytecodeCallFunction::BytecodeCallFunction(std::string callName)
{
	this->operation = BytecodeOp::call;
	this->callName = callName;
}

std::string BytecodeCallFunction::toString()
{
	return "\tCALL " + this->callName;
}

void ds::BytecodeCallFunction::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue(compiler->functions[this->callName].offset);
}

bytecodeOffset ds::BytecodeCallFunction::getArgsSize()
{
	return sizeof(bytecodeOffset);
}

// --------------- //
// FunctionAddress //
// --------------- //

ds::BytecodeFunctionAddress::BytecodeFunctionAddress(std::string callName, bool native)
{
	this->operation = BytecodeOp::push;
	this->callName = callName;
	this->native = native;
}

std::string BytecodeFunctionAddress::toString()
{
	return "\tPUSH ADDROFF {}" + this->callName;
}

void ds::BytecodeFunctionAddress::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
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

bytecodeOffset ds::BytecodeFunctionAddress::getArgsSize()
{
	return sizeof(bytecodeOffset);
}

// ------------- //
// CallNative    //
// ------------- //

ds::BytecodeCallNative::BytecodeCallNative(NativeFunction* function)
{
	this->functionName = function->getFullName();
	this->operation = BytecodeOp::callExternal;
}

ds::BytecodeCallNative::BytecodeCallNative(std::string functionName)
{
	this->operation = BytecodeOp::callExternal;
	this->functionName = functionName;
}

std::string BytecodeCallNative::toString()
{
	return "\tNATIVE_CALL " + functionName;
}

void ds::BytecodeCallNative::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
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

bytecodeOffset ds::BytecodeCallNative::getArgsSize()
{
	return sizeof(uint32_t);
}

// ------------- //
// JumpLabel     //
// ------------- //

ds::BytecodeJumpLabel::BytecodeJumpLabel(std::string name)
{
	this->name = name;
	this->baseSize = 0;
}

void ds::BytecodeJumpLabel::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
}

bytecodeOffset ds::BytecodeJumpLabel::getArgsSize()
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

ds::BytecodeJump::BytecodeJump(BytecodeOp operation, BytecodeJumpLabel* target)
{
	this->operation = operation;
	this->target = target;
}

void ds::BytecodeJump::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue<bytecodeOffset>(this->target->offset);
}

bytecodeOffset ds::BytecodeJump::getArgsSize()
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

ds::BytecodeAllocClass::BytecodeAllocClass(ClassType* languageClass)
{
	this->languageClass = languageClass;
	this->operation = BytecodeOp::allocClass;
}
void ds::BytecodeAllocClass::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	// typeid
	stream.addValue(uint32_t(0));
	// vtable
	stream.addValue(languageClass->vTableOffset);
}

bytecodeOffset ds::BytecodeAllocClass::getArgsSize()
{
	return sizeof(uint32_t) * 2;
}

std::string BytecodeAllocClass::toString()
{
	return "\tALLOC " + this->languageClass->name;
}

// ------------- //
// Await         //
// ------------- //

ds::BytecodeAwait::BytecodeAwait(Size awaitSize, BytecodeJumpLabel* onFinished)
{
	this->operation = BytecodeOp::awaitTask;
	this->target = onFinished;
	this->awaitSize = awaitSize;
}

void ds::BytecodeAwait::getArgs(BinaryBuffer& stream, BytecodeCompiler* compiler)
{
	stream.addValue<Size>(this->awaitSize);
	stream.addValue<bytecodeOffset>(this->target->offset);
}

bytecodeOffset ds::BytecodeAwait::getArgsSize()
{
	return sizeof(Size) + sizeof(bytecodeOffset);
}

std::string ds::BytecodeAwait::toString()
{
	return "\tAWAIT_TASK " + std::to_string(this->awaitSize) + " -> " + this->target->name;
}

void ds::BytecodeCompiler::printAssembly()
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
			std::printf("%lu %s", instr->offset, instr->toString().c_str());
#endif
		}
	}
}

void ds::BytecodeCompiler::compileTo(BytecodeStream& stream, std::vector<Function*> virtualTable, ErrorContext* errors)
{
	bytecodeOffset bytecodePos = stream.code.streamPos;

	std::vector<BytecodeFunction*> orderedFunctions;

	for (auto& i : this->functions)
	{
		i.second.name = i.first;
		if (i.second.isPreCompiled)
		{
			continue;
		}
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
		if (i && functions.find(i->getFullName()) != functions.end())
		{
			stream.virtualTable.push_back(RuntimeFunction{
				.codeOffset = functions[i->getFullName()].offset });
		}
		else
		{
			stream.virtualTable.push_back(RuntimeFunction());
		}
	}

	BinaryBuffer argsBuffer;
	for (auto& code : orderedFunctions)
	{
		stream.debug.sections.push_back(DebugSection{
			.offset = code->offset,
			.name = code->name,
		});

		auto& s = stream.unwind.sections.emplace_back(code->offset);

		for (auto& instr : code->instructions)
		{
			instr->addUnwindInfo(this, s);
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

void ds::BytecodeBuffer::add(InstructionPtr instruction)
{
	this->instructions.push_back(instruction);
}

void ds::BytecodeBuffer::pushInt(uint32_t data)
{
	static BinaryBuffer args;
	args.clear();
	args.addValue(data);
	addOperation(BytecodeOp::push, args);
}

void ds::BytecodeBuffer::addOperation(BytecodeOp operation, const BinaryBuffer& arguments)
{
	add(std::make_shared<BytecodeOperation>(operation, arguments));
}

void ds::BytecodeBuffer::addOperation(BytecodeOp operation)
{
	add(std::make_shared<BytecodeOperation>(operation, BinaryBuffer()));
}

void ds::BytecodeBuffer::prependBuffer(const BytecodeBuffer& other)
{
	this->instructions.reserve(this->instructions.size() + other.instructions.size());

#ifdef _WIN32
	// GCC doesn't have this function yet, cool
	this->instructions.insert_range(instructions.begin(), other.instructions);
#else
//	this->instructions.insert(instructions.begin(), other.instructions);
#endif
}
void ds::BytecodeBuffer::addBuffer(const BytecodeBuffer& other)
{
	this->instructions.reserve(this->instructions.size() + other.instructions.size());

	for (auto& instr : other.instructions)
	{
		this->instructions.push_back(instr);
	}
}
