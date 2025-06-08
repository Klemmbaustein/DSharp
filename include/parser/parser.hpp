#pragma once
#include "attribute.hpp"
#include "bytecode/compileBytecode.hpp"
#include "expression.hpp"
#include "parseClass.hpp"
#include "function.hpp"
#include "module.hpp"
#include "tokens.hpp"
#include "types/type.hpp"
#include <bytecode.hpp>
#include <map>
#include <string>
#include <vector>
#include "error.hpp"

namespace lang
{
	struct LanguageContext;

	struct ParseContext
	{
		ParseContext(LanguageContext* context);

		void addFile(std::string filePath);

		BytecodeStream compile();

		struct ParsedVariable
		{
			Type* type;
			Token name;
			std::vector<Token> value;
		};

		friend struct ParsedFunction;
		friend struct ParsedFile;
		friend struct ParsedScope;

		ErrorContext errors;
		BytecodeCompiler compiler;
		std::vector<Function*> virtualTable;

	private:
		void scanModules();

		std::map<std::string, Module> programModules;
		std::vector<Type*> defaultTypes;
		std::vector<ParsedFile> files;
	};

	struct ParsedFile;

	struct ParsedFunction : public Function
	{
		Token name;
		TokenPos start;
		TokenPos end;
		ParsedClass* inClass = nullptr;
		BytecodeFunction functionCode;
		Module* functionModule = nullptr;
		ParsedFile* functionFile = nullptr;
		Type* returnType = nullptr;
		std::vector<FunctionArgument> arguments;

		bool isVirtual = false;
		bool isOverride = false;
		bytecodeOffset vTableOffset = 0;

		std::string getFullName() const override;

		void scanDeclaration(TokenLine currentLine, TokenStream& stream, ParsedFile* file, ErrorContext* errors);

		TokenStream functionStream;
		std::vector<Token> returnTypeTokens;
		std::vector<Token> argumentTokens;

		void resolveTypes(ParseContext* context, ErrorContext* errors);
		void compile(ParseContext* context, ParsedFile* file, ErrorContext* errors);

		virtual ExpressionResult compileCall();
		virtual std::vector<FunctionArgument> getArguments();
		virtual bool discardable() const override;
	};

	struct ParsedClassMethod : public ParsedFunction
	{
	};

	struct ParsedFile : Attributable
	{
		TokenStream stream;
		std::vector<ParsedFunction> functions;
		std::vector<ParsedClass> classes;
		std::map<Token, Module*> usings;
		Module* fileModule = nullptr;

		std::string name;
		std::string scopeName;

		void loadAvailableTypes(ParseContext* context);

		Function* getMethod(std::string name);
		Type* getType(TokenLine& from);
		Attribute* getAttribute(TokenLine& from);

		void scan(ErrorContext* errors);
		bool scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors);
		ParsedFunction& scanFunction(TokenLine currentLine, ErrorContext* errors);
		ParsedClass& scanClass(TokenLine currentLine, ErrorContext* errors);

		void compile(ParseContext* context);
	};
} // namespace lang