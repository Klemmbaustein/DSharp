#pragma once
#include "classType.hpp"

namespace lang
{
	class StringType : public ClassType
	{
	public:
		StringType()
		{
			this->name = "string";
			this->size = sizeof(size_t);
		}

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors, ParsedScope* with) override;
		virtual ExpressionResult compileCast(ExpressionResult value) override;
	};
}