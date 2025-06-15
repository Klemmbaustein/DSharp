#include <parser/types/listType.hpp>
#include <parser/parseScope.hpp>
#include <parser/types/arrayType.hpp>
using namespace lang;

ExpressionResult lang::ListType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with)
{
	if (first != "[")
	{
		return ExpressionResult();
	}

	auto inList = line.getUntil("]", errors);

	TokenLine inListLine;
	inListLine.lineTokens = &inList;

	Type* itemType = nullptr;

	std::vector<ExpressionResult> arrayElements;

	while (!inListLine.empty())
	{
		auto nextValue = with->pushExpression(inListLine, errors, false);
		if (itemType == nullptr)
		{
			itemType = nextValue.type;
		}
		else if (!itemType->sameAs(nextValue.type))
		{
			errors->error(ErrorCode::parseInvalidType, inListLine.previous(),
				"Initializer list type mismatch. Expected " +
					Type::toString(itemType) + ", got " + Type::toString(nextValue.type));
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

	if (!itemType)
	{
		ExpressionResult result;
		result.valid = true;
		result.type = this;
		return result;
	}

	ArrayType* arrayType = ArrayType::getInstance(itemType);

	return arrayType->makeArrayValue(arrayElements, with);
}

ExpressionResult lang::ListType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult lang::ListType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}
