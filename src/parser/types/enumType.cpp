#include <parser/types/enumType.hpp>
using namespace lang;

ExpressionResult lang::EnumType::compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult EnumType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	return ExpressionResult();
}

ExpressionResult lang::EnumType::compileCast(ExpressionResult value, ParsedScope* with)
{
	if (value.type->sameAs(IntType::getInstance()))
	{
		value.type = IntType::getInstance();
		return value;
	}

	return ExpressionResult();
}
