#include <ds/parser/types/enumType.hpp>
#include <ds/parser/parseScope.hpp>
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
	IntType* intType = nullptr;

	if (with->context->registry->ifTypeIs<IntType>(value.type, intType))
	{
		value.type = intType;
		return value;
	}

	return ExpressionResult();
}
