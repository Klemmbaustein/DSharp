#pragma once
#include "parser.hpp"
#include "parseClass.hpp"
#include "bytecode/compileBytecodeVariables.hpp"

namespace lang
{
	struct ScopeVariable
	{
		Token name;
		std::shared_ptr<BytecodePushVariable> variableInstruction = nullptr;
		ParsedScope* ownedBy = nullptr;
		size_t depth = 0;
		Type* type = nullptr;
		bool readOnly = false;
		bool isInternal = false;
		mutable uint32_t lambdaOffset = 0;

		BytecodeBuffer readValue(ParsedScope* with) const;
		BytecodeBuffer writeValue() const;
		ExpressionResult readExpression(ParsedScope* with) const;
	};

	struct ParsedScope
	{
		ParsedFunction* scopeFunction = nullptr;
		ParsedFile* scopeFile = nullptr;
		BytecodeBuffer* code = nullptr;
		ParsedClass* inClass = nullptr;
		ParseContext* context = nullptr;
		TokenStream* tokenStream = nullptr;

		size_t depth = 0;

		std::map<Token, ScopeVariable> variables;

		BytecodeBuffer addTemporaryVariable(Type* type);

		ScopeVariable* thisVariable = nullptr;
		ScopeVariable* lambdaVariable = nullptr;

		void pushVariableValue(Type* type, bool copy);
		ScopeVariable& addVariable(Token name, Type* type);
		void compileScopeExit(size_t toDepth, bool isEnd);

		uint32_t variableStackPosition = 0;
		uint32_t lambdaOffset = 0;
		bool compileReturn = false;
		bool returnThis = false;
		bool isLambda = false;

		std::shared_ptr<BytecodeJumpLabel> breakTarget = nullptr;
		std::shared_ptr<BytecodeJumpLabel> continueTarget = nullptr;
		size_t breakContinueDepth = 0;

		void setClass(ParsedClass* inClass, bool copy);

		void compile(ParseContext* context, ParsedFile* file, ErrorContext* errors);
		void compileLine(TokenLine line, ParsedFile* file, ErrorContext* errors);
		void compileIf(TokenLine line, ParsedFile* file, ErrorContext* errors);
		void compileFor(TokenLine line, ParsedFile* file, ErrorContext* errors);
		ExpressionResult pushExpression(TokenLine& currentLine, ErrorContext* errors,
			bool setExpression, Type* hintType);
		ExpressionResult getExpressionValue(TokenLine& currentLine, ErrorContext* errors,
			bool setExpression, Type* hintType);
		ExpressionResult compileOperatorBetween(ExpressionResult a, ExpressionResult b, Operator op, Token opToken,
			ErrorContext* errors, bool setExpression);
		ExpressionResult pushValue(TokenLine& currentLine, ErrorContext* errors,
			bool setExpression, Type* hintType);
		ExpressionResult pushClassValue(TokenLine& currentLine, ErrorContext* errors, bool setExpression);

		struct ScopeOptions
		{
			BytecodeBuffer* targetBuffer = nullptr;
			TokenStream* scopeTokens = nullptr;
			ParsedFunction* scopeFunction = nullptr;
			bool isLambda = false;
		};

		void parseSubScope(ParsedFile* file, ErrorContext* errors, std::shared_ptr<BytecodeJumpLabel> breakTarget,
			std::shared_ptr<BytecodeJumpLabel> continueTarget, size_t breakContinueDepth, ScopeOptions options = ScopeOptions());

		ExpressionResult parseFunctionArguments(std::string functionName, std::vector<FunctionArgument> arguments,
			TokenLine& currentLine, ErrorContext* errors);

	private:

		uint32_t tempCounter = 0;
	};
} // namespace lang