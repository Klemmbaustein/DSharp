#pragma once
#include "type.hpp"

namespace ds
{
	class EnumType : public Type
	{
	public:
		EnumType()
		{
			this->size = sizeof(Int);
			this->hasDefaultValue = false;
		}
		std::map<Token, int32_t> values;

		ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;
		ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
	};
}