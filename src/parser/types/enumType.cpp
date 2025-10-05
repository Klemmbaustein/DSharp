#include <ds/parser/types/enumType.hpp>
using namespace ds;

ExpressionResult ds::EnumType::compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult EnumType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	return ExpressionResult();
}

ExpressionResult ds::EnumType::compileCast(ExpressionResult value, ParsedScope* with)
{
	if (value.type->sameAs(IntType::getInstance()))
	{
		value.type = IntType::getInstance();
		return value;
	}

	return ExpressionResult();
}
