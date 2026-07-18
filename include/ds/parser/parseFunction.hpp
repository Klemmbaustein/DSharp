#pragma once
#include "function.hpp"
#include "parseClass.hpp"
#include <iostream>

namespace ds
{
	struct ParsedFile;

	struct ParsedFunction : public Function
	{
		Token name;
		TokenPos start;
		TokenPos end;
		ParsedClass* inClass = nullptr;
		BytecodeFunction* functionCode = nullptr;
		Module* functionModule = nullptr;
		ParsedFile* functionFile = nullptr;
		Type* returnType = nullptr;
		std::vector<FunctionArgument> arguments;
		std::vector<Token> capturedVariables;

		bool functionIsVirtual = false;
		bool isOverride = false;
		bool foundOverride = false;
		bool isLambda = false;
		bool isAsync = false;
		bool inheritsGeneric = false;
		BytecodeOffset vTableOffset = 0;

		std::string getFullName() const override;
		std::string getShortName() const override;

		void registerFunction(ParseContext* context);

		void scanDeclaration(TokenLine currentLine, TokenStream& stream, ParsedFile* file, ErrorContext* errors);

		void addArguments(ParsedScope& scope, ErrorContext* errors);

		TokenStream functionStream;
		std::vector<Token> returnTypeTokens;
		std::vector<Token> argumentTokens;

		void resolveTypes(ParseContext* context, ErrorContext* errors);
		void compile(ParseContext* context, ParsedFile* file, ErrorContext* errors);

		ExpressionResult compileCall();
		std::vector<FunctionArgument> getArguments();
		Type* getReturnType() override;
		bool discardable() const override;
		bool isVirtual() const
		{
			return functionIsVirtual;
		}

		BytecodeBuffer compileCallable(ErrorContext* errors, ParsedScope* with, Type* hintType) const override;
		std::optional<SymbolDefinition> getDefinition() override;

		virtual BytecodeOffset getVirtualOffset() const
		{
			return vTableOffset;
		}
	};

}