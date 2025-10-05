#include <ds/parser/types/listType.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/types/arrayType.hpp>
using namespace ds;

ExpressionResult ds::ListType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	if (first != "[")
	{
		return ExpressionResult();
	}

	auto inList = line.getUntil("]", errors);

	TokenLine inListLine;
	inListLine.lineTokens = &inList;

	Type* itemType = nullptr;

	auto arrayHint = dynamic_cast<ArrayType*>(hintType);

	if (arrayHint)
	{
		itemType = arrayHint->baseType;
	}

	std::vector<ExpressionResult> arrayElements;

	while (!inListLine.empty())
	{
		auto nextValue = with->pushExpression(inListLine, errors, false, itemType);

		if (!nextValue.type && nextValue.valid)
		{
			errors->error(ErrorCode::parseInvalidType, inListLine.previous(), "Expected a type");
		}

		if (!nextValue.valid)
		{
			break;
		}

		if (itemType == nullptr)
		{
			itemType = nextValue.type;
		}
		else if (!itemType->sameAs(nextValue.type))
		{
			nextValue.compileToType(first, itemType, with, errors);
		}

		arrayElements.push_back(nextValue);

		auto next = inListLine.get();

		if (next.empty())
		{
			break;
		}
		else if (next == ",")
		{
			continue;
		}
		else
		{
			errors->error(ErrorCode::parseInvalidType, inListLine.previous(),
				"Expected a comma, got '" + next.string + "'");
		}
	}

	if (arrayElements.empty() || !itemType)
	{
		ExpressionResult result;
		result.valid = true;
		result.type = this;
		return result;
	}

	ArrayType* arrayType = ArrayType::getInstance(itemType);

	return arrayType->makeArrayValue(arrayElements, with);
}

ExpressionResult ds::ListType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::ListType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}
