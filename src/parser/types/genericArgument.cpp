#include <ds/parser/types/genericArgument.hpp>

using namespace ds;

ds::GenericArgumentType::GenericArgumentType(size_t index, bool isFunctionIndex, bool isNullable)
{
	this->name = "T" + std::to_string(index) + (isNullable ? "?" : "");
	this->index = index;
	this->isFunctionIndex = isFunctionIndex;

	if (!isNullable)
	{
		this->nullable = new GenericArgumentType(index, isFunctionIndex, true);
		this->nullable->isNullable = true;
	}
}

ds::GenericArgumentType::~GenericArgumentType()
{
	if (this->nullable)
	{
		delete this->nullable;
	}
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