#pragma once
#include "parser.hpp"
#include "parseClass.hpp"
#include "bytecode/compileBytecodeVariables.hpp"

namespace lang
{
	struct ParsedScope
	{
		ParsedFunction* scopeFunction = nullptr;
		ParsedFile* scopeFile = nullptr;
		BytecodeBuffer* code = nullptr;
		ParsedClass* inClass = nullptr;
		ParseContext* context = nullptr;
		TokenStream* tokenStream = nullptr;

		struct ScopeVariable
		{
			Token name;
			BytecodePushVariable* variableInstruction = nullptr;
			ParsedScope* ownedBy = nullptr;
			Type* type = 0;

			BytecodeBuffer readValue(ParsedScope* scope) const;
			BytecodeBuffer writeValue(ParsedScope* scope) const;
		};

		std::map<Token, ScopeVariable> variables;

		BytecodeBuffer addTemporaryVariable(Type* type);

		ScopeVariable* thisVariable = nullptr;

		void pushVariableValue(Type* type, bool copy);
		ScopeVariable& addVariable(Token name, Type* type);
		void compileScopeExit(bool full);

		uint32_t variableStackPosition = 0;
		bool compileReturn = false;
		bool returnThis = false;

		void setClass(ParsedClass* inClass, bool copy);

		void compile(ParseContext* context, ParsedFile* file, ErrorContext* errors);
		void compileLine(TokenLine line, ParsedFile* file, ErrorContext* errors);
		void compileIf(TokenLine line, ParsedFile* file, ErrorContext* errors);
		ExpressionResult pushExpression(TokenLine& currentLine, ErrorContext* errors, bool setExpression);
		ExpressionResult pushValue(TokenLine& currentLine, ErrorContext* errors, bool setExpression);
		ExpressionResult pushClassValue(TokenLine& currentLine, ErrorContext* errors, bool setExpression);

		void parseSubScope(ParsedFile* file, ErrorContext* errors);

		ExpressionResult parseFunctionArguments(std::string functionName, std::vector<FunctionArgument> arguments,
			TokenLine& currentLine, ErrorContext* errors);

	private:
		uint32_t tempCounter = 0;
	};
} // namespace lang