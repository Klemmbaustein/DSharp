#pragma once
#include "parseScope.hpp"

namespace ds
{
	class Expression
	{
	public:
		static ExpressionResult pushExpression(TokenLine& currentLine, ErrorContext* errors,
			bool setExpression, Type* hintType, ParsedScope* scope);
		static ExpressionResult getExpressionValue(TokenLine& currentLine, ErrorContext* errors,
			bool setExpression, Type* hintType, ParsedScope* scope);
		static ExpressionResult compileOperatorBetween(ExpressionResult a, ExpressionResult b, Operator op, Token opToken,
			ErrorContext* errors, bool setExpression, ParsedScope* scope);
		static ExpressionResult pushValue(TokenLine& currentLine, ErrorContext* errors,
			bool setExpression, Type* hintType, ParsedScope* scope);
		static ExpressionResult pushClassValue(TokenLine& currentLine, ErrorContext* errors, bool setExpression,
			ParsedScope* scope);

		static ExpressionResult parseFunctionArguments(Token functionName, std::vector<FunctionArgument> arguments,
			TokenLine& currentLine, ErrorContext* errors, bool hasToMatch, ParsedScope* scope);
	};
}