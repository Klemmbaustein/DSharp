#pragma once
#include "classType.hpp"

namespace ds
{
	class StringType : public ClassType
	{
	public:
		StringType()
		{
			this->name = "string";
			this->size = sizeof(Pointer);
			this->vTableOffset = UINT32_MAX;
			applyName();
		}

		ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors,
			ParsedScope* with, Type* hintType) override;
		ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;
		ExpressionResult compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;

		ExpressionResult compileEqualsTo(ExpressionResult first, ExpressionResult second, Token opToken,
			ErrorContext* errors, ParsedScope* with) override;

		static StringType* getInstance()
		{
			if (!instance)
			{
				instance = new StringType();
			}
			return instance;
		}

	private:
		static inline StringType* instance = nullptr;

		ExpressionResult compileFormatString(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with);

		ExpressionResult compileStringValue(std::string str, ParsedScope* with);
	};
} // namespace ds