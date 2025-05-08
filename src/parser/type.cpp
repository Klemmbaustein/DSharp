#include <parser/type.hpp>
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

ExpressionResult lang::IntType::compileValue(Token first, TokenLine& line, ErrorContext* errors, ParsedScope* with)
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

ExpressionResult lang::Type::compileMember(ExpressionResult value, TokenLine& line, ErrorContext* errors, bool setMember, ParsedScope* with)
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

ExpressionResult lang::FloatType::compileValue(Token first, TokenLine& line, ErrorContext* errors, ParsedScope* with)
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
