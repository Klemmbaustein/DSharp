#pragma once
#include "type.hpp"

namespace lang
{
	class ListType : public Type
	{
	public:
		ListType()
		{
			this->name = "<initializer list>";
			this->size = sizeof(Pointer);
		}

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;

		static ListType* getInstance()
		{
			if (!instance)
			{
				instance = new ListType();
			}
			return instance;
		}

	private:
		static inline ListType* instance = nullptr;
	};
} // namespace lang