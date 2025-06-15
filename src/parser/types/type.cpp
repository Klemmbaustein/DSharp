#include <parser/types/type.hpp>
#include <parser/stringUtils.hpp>
#include <parser/parseScope.hpp>
#include <parser/types/stringType.hpp>
using namespace lang;

ExpressionResult lang::IntType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	ExpressionResult result;
	auto floatValue = dynamic_cast<FloatType*>(second.type);
	if (floatValue)
	{
		ExpressionResult firstCopy = first;
		firstCopy.type = second.type;
		firstCopy.code.addOperation(BytecodeOp::intToFloat);
		result.type = firstCopy.type;
		return firstCopy.type->compileOperator(operatorType, firstCopy, second, with);
	}

	if (operatorType != Operator::less)
	{
		result.code.addBuffer(first.code);
		result.code.addBuffer(second.code);
	}
	else
	{
		result.code.addBuffer(second.code);
		result.code.addBuffer(first.code);
	}
	result.type = this;
	result.valid = true;

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
	case lang::Operator::less:
	case lang::Operator::greater:
		result.code.addOperation(BytecodeOp::greaterInt);
		result.type = BoolType::getInstance();
		break;
	case lang::Operator::modulo:
	case lang::Operator::unknown:
	default:
		return ExpressionResult();
	}

	return result;
}

ExpressionResult lang::IntType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with)
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

ExpressionResult lang::IntType::compileCast(ExpressionResult value, ParsedScope* with)
{
	auto floatValue = dynamic_cast<FloatType*>(value.type);
	if (floatValue)
	{
		value.code.addOperation(BytecodeOp::floatToInt);
		return value;
	}

	return ExpressionResult();
}

ExpressionResult lang::IntType::compileToString(ExpressionResult thisValue, ErrorContext* errors, ParsedScope* with)
{
	ExpressionResult result = thisValue;
	result.code.add(new BytecodeCallNative("system::int.toString"));
	result.valid = true;
	result.type = StringType::getInstance();
	return result;
}

ExpressionResult lang::Type::defaultValue()
{
	ExpressionResult result;
	result.valid = true;
	result.type = this;

	BinaryBuffer args;
	for (uint32_t i = 0; i < this->size; i++)
	{
		args.addValue<char>(0);
	}

	result.code.addOperation(BytecodeOp::push, args);
	return result;
}

ExpressionResult lang::Type::compileMember(ExpressionResult value, TokenLine& line,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult lang::Type::compileEqualsTo(ExpressionResult first, ExpressionResult second)
{
	if (!first.type->sameAs(second.type))
	{
		return ExpressionResult();
	}

	ExpressionResult result;
	result.valid = true;
	result.type = first.type;

	result.code.addBuffer(first.code);
	result.code.addBuffer(second.code);

	result.code.pushInt(first.type->size);
	result.code.addOperation(BytecodeOp::equals);

	return result;
}

ExpressionResult lang::Type::compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
	ErrorContext* errors, bool setMember, ParsedScope* scope)
{
	return ExpressionResult();
}

ExpressionResult lang::Type::compileToString(ExpressionResult thisValue, ErrorContext* errors, ParsedScope* with)
{
	return ExpressionResult();
}

std::string lang::Type::toString(Type* target)
{
	return target ? target->name : "<void>";
}

ExpressionResult lang::FloatType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{

	ExpressionResult result;

	if (operatorType != Operator::less)
	{
		result.code.addBuffer(first.code);
		result.code.addBuffer(second.code);
		auto intValue = dynamic_cast<IntType*>(second.type);
		if (intValue)
		{
			result.code.addOperation(BytecodeOp::intToFloat);
		}
	}
	else
	{
		result.code.addBuffer(second.code);
		auto intValue = dynamic_cast<IntType*>(second.type);
		if (intValue)
		{
			result.code.addOperation(BytecodeOp::intToFloat);
		}
		result.code.addBuffer(first.code);
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
	case lang::Operator::less:
	case lang::Operator::greater:
		result.code.addOperation(BytecodeOp::greaterFloat);
		result.type = BoolType::getInstance();
		break;

	case lang::Operator::modulo:
	case lang::Operator::unknown:
	default:
		return ExpressionResult();
	}

	result.type = this;
	result.valid = true;
	return result;
}

ExpressionResult lang::FloatType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with)
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

ExpressionResult lang::FloatType::compileCast(ExpressionResult value, ParsedScope* with)
{
	auto intValue = dynamic_cast<IntType*>(value.type);
	if (intValue)
	{
		value.code.addOperation(BytecodeOp::intToFloat);
		return value;
	}

	return ExpressionResult();
}

ExpressionResult lang::FloatType::compileToString(ExpressionResult thisValue, ErrorContext* errors, ParsedScope* with)
{
	ExpressionResult result = thisValue;
	result.code.add(new BytecodeCallNative("system::float.toString"));
	result.valid = true;
	result.type = StringType::getInstance();
	return result;
}

ExpressionResult lang::BoolType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	ExpressionResult result;

	result.code.addBuffer(first.code);
	result.code.addBuffer(second.code);

	switch (operatorType)
	{
	case lang::Operator::logicalAnd:
		result.code.addOperation(BytecodeOp::boolAnd);
		break;
	default:
		return ExpressionResult();
	}

	result.type = this;
	result.valid = true;
	return result;
}

ExpressionResult lang::BoolType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with)
{
	bool value = false;
	if (first == "true")
	{
		value = true;
	}
	else if (first != "false")
	{
		return ExpressionResult();
	}

	ExpressionResult result;
	BinaryBuffer valueBuffer;
	valueBuffer.addValue<bool>(value);
	result.code.addOperation(BytecodeOp::push, valueBuffer);
	result.valid = true;
	result.type = this;
	return result;
}

ExpressionResult lang::BoolType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult lang::CharType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	ExpressionResult result;

	result.code.addBuffer(first.code);
	result.code.addBuffer(second.code);

	switch (operatorType)
	{
	case lang::Operator::logicalAnd:
		result.code.addOperation(BytecodeOp::boolAnd);
		break;
	default:
		return ExpressionResult();
	}

	result.type = this;
	result.valid = true;
	return result;
}

ExpressionResult lang::CharType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with)
{
	if (first.string[0] != '\'')
	{
		return ExpressionResult();
	}

	ExpressionResult result;
	BinaryBuffer valueBuffer;
	valueBuffer.addValue<char>(first.string[1]);
	result.code.addOperation(BytecodeOp::push, valueBuffer);
	result.valid = true;
	result.type = this;
	return result;
}

ExpressionResult lang::CharType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}
