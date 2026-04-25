#pragma once
#include "classType.hpp"

namespace ds
{
	class StringType : public ClassType
	{
	public:
		StringType(TypeRegistry* registry);

		ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors,
			ParsedScope* with, Type* hintType) override;
		ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		ExpressionResult compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;

		ExpressionResult compileEqualsTo(ExpressionResult first, ExpressionResult second, Token opToken,
			ErrorContext* errors, ParsedScope* with) override;

		static constexpr TypeId STRING_ID = typeIdFromName("string");

	private:
		ExpressionResult compileFormatString(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with);

		ExpressionResult compileStringValue(std::string str, ParsedScope* with);
	};
} // namespace ds