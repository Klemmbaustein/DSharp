#pragma once
#include "parser.hpp"

namespace lang
{
	struct ParsedScope
	{
		ParsedFunction* scopeFunction = nullptr;
		ParsedFile* scopeFile = nullptr;
		BytecodeBuffer* code = nullptr;
		ParseContext* context = nullptr;
		TokenStream* tokenStream = nullptr;

		struct ScopeVariable
		{
			Token name;
			uint32_t stackPosition = 0;
			ParsedScope* ownedBy = nullptr;
			Type* type = 0;

			uint32_t getRelativePosition(ParsedScope* scope) const;

			BytecodeBuffer readValue(ParsedScope* scope) const;
			BytecodeBuffer writeValue(ParsedScope* scope) const;
		};

		std::map<Token, ScopeVariable> variables;

		void pushVariableValue(Type* type);
		ScopeVariable& addVariable(Token name, Type* type);

		void compileScopeExit(bool full);

		uint32_t variableStackPosition = 0;

		void compile(ParseContext* context, ParsedFile* file, ErrorContext* errors);
		void compileLine(TokenLine line, ParsedFile* file, ErrorContext* errors);
		ExpressionResult pushExpression(TokenLine& currentLine, ErrorContext* errors, bool setExpression);
		ExpressionResult pushValue(TokenLine& currentLine, ErrorContext* errors, bool setExpression);
	};
} // namespace lang