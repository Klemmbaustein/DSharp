#include <parser/type.hpp>
#include <parser/function.hpp>
#include <parser/stringUtils.hpp>
using namespace lang;

ExpressionResult lang::IntType::compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second)
{
	ExpressionResult result;

	result.code.addBuffer(first.code);

	auto floatValue = dynamic_cast<FloatType*>(second.type);
	if (floatValue)
	{
		ExpressionResult firstCopy = first;
		firstCopy.type = second.type;
		firstCopy.code.addOperation(BytecodeOp::intToFloat);
		result.type = firstCopy.type;
		return firstCopy.type->compileOperator(operatorType, firstCopy, second);
	}

	result.code.addBuffer(second.code);

	switch (operatorType)
	{
	case lang::Operator::add:
		result.code.addOperation(BytecodeOp::addInt);
		break;
	case lang::Operator::subtract:
		result.code.addOperation(BytecodeOp::subInt);
		break;
	case lang::Operator::multiply:
		result.code.addOperation(BytecodeOp::mulInt);
		break;
	case lang::Operator::divide:
		result.code.addOperation(BytecodeOp::divInt);
		break;
	case lang::Operator::modulo:
	case lang::Operator::unknown:
		return ExpressionResult();
	default:
		break;
	}

	result.type = this;
	result.valid = true;
	return result;
}

ExpressionResult lang::IntType::compileValue(Token first, TokenLine& line)
{
	bool isNegative = false;
	if (first == "+")
	{
		first = line.get();
	}
	else if (first == "-")
	{
		first = line.get();
		isNegative = true;
	}

	if (!isNumber(first.string.c_str(), false))
	{
		return ExpressionResult();
	}

	int32_t number = std::stoi(first.string);

	if (isNegative)
	{
		number = -number;
	}

	ExpressionResult result;
	BinaryBuffer valueBuffer;
	valueBuffer.addValue(number);
	result.code.addOperation(BytecodeOp::push, valueBuffer);
	result.valid = true;
	result.type = this;
	return result;
}

ExpressionResult lang::IntType::compileCast(ExpressionResult value)
{
	auto floatValue = dynamic_cast<FloatType*>(value.type);
	if (floatValue)
	{
		value.code.addOperation(BytecodeOp::floatToInt);
		return value;
	}

	return ExpressionResult();
}

ExpressionResult lang::Type::compileMember(ExpressionResult value, TokenLine& line, ErrorContext* errors, bool setMember)
{
	return ExpressionResult();
}

std::string lang::Type::toString(Type* target)
{
	return target ? target->name : "<void>";
}

ExpressionResult lang::FloatType::compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second)
{
	ExpressionResult result;

	result.code.addBuffer(first.code);
	result.code.addBuffer(second.code);

	auto intValue = dynamic_cast<IntType*>(second.type);
	if (intValue)
	{
		result.code.addOperation(BytecodeOp::intToFloat);
	}

	switch (operatorType)
	{
	case lang::Operator::add:
		result.code.addOperation(BytecodeOp::addFloat);
		break;
	case lang::Operator::subtract:
		result.code.addOperation(BytecodeOp::subFloat);
		break;
	case lang::Operator::multiply:
		result.code.addOperation(BytecodeOp::mulFloat);
		break;
	case lang::Operator::divide:
		result.code.addOperation(BytecodeOp::divFloat);
		break;
	case lang::Operator::modulo:
	case lang::Operator::unknown:
		return ExpressionResult();
	default:
		break;
	}

	result.type = this;
	result.valid = true;
	return result;
}

ExpressionResult lang::FloatType::compileValue(Token first, TokenLine& line)
{
	bool isNegative = false;
	if (first == "+")
	{
		first = line.get();
	}
	else if (first == "-")
	{
		first = line.get();
		isNegative = true;
	}

	if (!isNumber(first.string.c_str(), true))
	{
		return ExpressionResult();
	}

	float number = std::stof(first.string);

	if (isNegative)
	{
		number = -number;
	}

	ExpressionResult result;
	BinaryBuffer valueBuffer;
	valueBuffer.addValue(number);
	result.code.addOperation(BytecodeOp::push, valueBuffer);
	result.valid = true;
	result.type = this;
	return result;
}

ExpressionResult lang::FloatType::compileCast(ExpressionResult value)
{
	auto intValue = dynamic_cast<IntType*>(value.type);
	if (intValue)
	{
		value.code.addOperation(BytecodeOp::intToFloat);
		return value;
	}

	return ExpressionResult();
}
ExpressionResult lang::ClassType::compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second)
{
	return ExpressionResult();
}

ExpressionResult lang::ClassType::compileValue(Token first, TokenLine& line)
{
	ExpressionResult result;

	auto constructorArgs = line.getInBraces();

	BinaryBuffer args;
	// class size
	args.addValue(uint32_t(this->classSize));
	// typeid
	args.addValue(uint32_t(0));
	result.code.addOperation(BytecodeOp::allocClass, args);
	result.code.addBuffer(this->constructor->compileCall().code);

	result.type = this;
	result.valid = true;

	return result;
}

ExpressionResult lang::ClassType::compileCast(ExpressionResult value)
{
	return ExpressionResult();
}

ExpressionResult lang::ClassType::compileMember(ExpressionResult value, TokenLine& line, ErrorContext* errors, bool setMember)
{
	Token memberName = line.get();

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
		args.addValue(i.offset);
		args.addValue(i.type->size);
		result.code.addBuffer(value.code);

		if (setMember)
		{
			result.setCode = result.code;
			result.setCode->addOperation(BytecodeOp::setClassMember, args);
		}

		result.code.addOperation(BytecodeOp::classMember, args);
		return result;
	}

	errors->error(ErrorCode::parseUnknowmMember, memberName, "The type " + this->name + " does not contain a member called '" + memberName.string + "'");

	return ExpressionResult();
}
