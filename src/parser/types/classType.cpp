#include <parser/types/classType.hpp>
#include <parser/function.hpp>
#include <parser/parseScope.hpp>

using namespace lang;

BytecodeBuffer lang::ClassType::compileUnref()
{
	BytecodeBuffer outBuffer;

	outBuffer.addOperation(BytecodeOp::unrefClass);

	return outBuffer;
}

BytecodeBuffer lang::ClassType::compileEndMove(ParsedScope* with)
{
	BytecodeBuffer outBuffer;
	BinaryBuffer args;
	args.addValue<uint32_t>(this->size);
	outBuffer.addOperation(BytecodeOp::copy, args);
	outBuffer.addBuffer(with->addTemporaryVariable(this));
	return outBuffer;
}

BytecodeBuffer lang::ClassType::compileMove(ParsedScope* with)
{
	BytecodeBuffer outBuffer;
	BinaryBuffer args;

	outBuffer.addOperation(BytecodeOp::refClass);

	return outBuffer;
}

ExpressionResult lang::ClassType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult lang::ClassType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with)
{
	ExpressionResult result;

	auto constructorArgs = line.getInBraces(errors);
	TokenLine argsLine;
	argsLine.lineTokens = &constructorArgs;

	if (this->constructors.empty())
	{
		result.code.pushInt(this->classSize);
		result.code.add(new BytecodeAllocClass(this));
		result.code.addBuffer(baseConstructor->compileCall().code);
	}
	else
	{
		for (auto& i : this->constructors)
		{
			result.code.addBuffer(with->parseFunctionArguments(i->getArguments(), argsLine, errors).code);

			result.code.pushInt(this->classSize);
			result.code.add(new BytecodeAllocClass(this));
			result.code.addBuffer(i->compileCall().code);
			break;
		}
	}
	result.type = this;
	result.valid = true;

	return result;
}

ExpressionResult lang::ClassType::compileCast(ExpressionResult value)
{
	return ExpressionResult();
}

ExpressionResult lang::ClassType::compileMember(ExpressionResult value, TokenLine& line,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	Token memberName = line.get();

	if (line.peek() == "(")
	{
		auto inBraces = line.getInBraces(errors);

		for (auto& [name, function] : this->methods)
		{
			if (name != memberName.string)
			{
				continue;
			}
			ExpressionResult result;
			result.valid = true;
			result.code.addBuffer(value.code);

			auto compiled = function->compileCall();
			if (compiled.type)
			{
				compiled.code.addBuffer(compiled.type->compileEndMove(with));
			}

			result.type = compiled.type;
			result.code.addBuffer(compiled.code);

			return result;
		}

		errors->error(ErrorCode::parseUnknowmMember, memberName,
			"The type " + this->name + " does not contain a method called '" + memberName.string + "'");

		return ExpressionResult();
	}

	for (auto& i : this->members)
	{
		if (i.name.string != memberName.string)
		{
			continue;
		}
		ExpressionResult result;
		result.valid = true;
		result.type = i.type;
		BinaryBuffer args;
		result.code.addBuffer(value.code);
		result.code.pushInt(i.offset);
		result.code.pushInt(i.type->size);

		if (setMember)
		{
			auto unrefCode = i.type->compileUnref();
			result.setCode = BytecodeBuffer();
			if (unrefCode.instructions.size())
			{
				result.setCode->addBuffer(result.code);
				result.setCode->addOperation(BytecodeOp::classMember, args);
				result.setCode->addBuffer(unrefCode);
			}

			result.setCode->addBuffer(result.code);
			result.setCode->addOperation(BytecodeOp::setClassMember, args);
		}

		result.code.addOperation(BytecodeOp::classMember, args);

		return result;
	}

	errors->error(ErrorCode::parseUnknowmMember, memberName,
		"The type " + this->name + " does not contain a member called '" + memberName.string + "'");

	return ExpressionResult();
}
