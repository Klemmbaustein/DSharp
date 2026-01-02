#include <ds/parser/types/genericArgument.hpp>

using namespace ds;

ds::GenericArgumentType::GenericArgumentType(size_t index, bool isFunctionIndex)
{
	this->index = index;
	this->isFunctionIndex = isFunctionIndex;
}

ExpressionResult ds::GenericArgumentType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::GenericArgumentType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	return ExpressionResult();
}

ExpressionResult ds::GenericArgumentType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}