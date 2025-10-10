#pragma once
#include "classType.hpp"
#include <ds/parser/parseScope.hpp>

namespace ds
{
	class LambdaType : public ClassType
	{
	public:
		LambdaType()
		{
			this->name = "<lambda>";
			this->size = sizeof(Pointer);
		}

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;

		static LambdaType* getInstance()
		{
			if (!instance)
			{
				instance = new LambdaType();
			}
			return instance;
		}

	private:

		ParsedFunction* compileDestructorFor(std::vector<ScopeVariable*> variables, Token lambdaName, ParsedScope* with);

		static inline LambdaType* instance = nullptr;

	};
}