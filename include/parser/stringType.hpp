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

		ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second) override;
		ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors, ParsedScope* with) override;
		ExpressionResult compileCast(ExpressionResult value) override;
		ExpressionResult compileMember(ExpressionResult value, TokenLine& line, ErrorContext* errors, bool setMember, ParsedScope* with) override;
	};
}