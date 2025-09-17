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
#include <list>
#include "parseEnum.hpp"

namespace lang
{
	struct LanguageContext;
	struct ScopeVariable;
	class LanguageService;

	struct ParseContext
	{
		ParseContext(LanguageContext* context);
		~ParseContext();

		void addFile(std::string filePath);
		void addString(const std::string& str, std::string fileName);
		void updateFile(const std::string& str, std::string fileName);

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
#ifdef WITH_LANGUAGE_SERVICE
		LanguageService* service = nullptr;
#endif

	private:
		void scanModules();

		std::map<std::string, Module> programModules;
		std::vector<Type*> defaultTypes;
		std::list<ParsedFile> files;
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
		std::vector<Token> capturedVariables;

		bool functionIsVirtual = false;
		bool isOverride = false;
		bool foundOverride = false;
		bool isLambda = false;
		bytecodeOffset vTableOffset = 0;

		std::string getFullName() const override;
		std::string getShortName() const override;

		void scanDeclaration(TokenLine currentLine, TokenStream& stream, ParsedFile* file, ErrorContext* errors);

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
		std::optional<FunctionDefinition> getDefinition() override
		{
			return FunctionDefinition{
				.file = functionFile,
				.at = name
			};
		}

		virtual bytecodeOffset getVirtualOffset() const
		{
			return vTableOffset;
		}
	};

	struct ParsedFile : Attributable
	{
		~ParsedFile();

		TokenStream stream;
		std::list<ParsedFunction> functions;
		std::list<ParsedClass> classes;
		std::list<ParsedEnum> enums;
		std::map<Token, Module*> usings;
		Module* fileModule = nullptr;
		ParseContext* context = nullptr;

		std::string name;
		std::string scopeName;

		void loadAvailableTypes(ParseContext* context);

		Function* getMethod(std::string name);
		Type* getType(TokenLine& from, ErrorContext* errors);
		Attribute* getAttribute(TokenLine& from);
		std::pair<EnumType*, std::string> getEnum(TokenLine& from);

		void scan(ErrorContext* errors);
		bool scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors);
		ParsedFunction& scanFunction(TokenLine currentLine, ErrorContext* errors);
		ParsedClass& scanClass(TokenLine currentLine, ErrorContext* errors);
		ParsedEnum& scanEnum(TokenLine currentLine, ErrorContext* errors);

		void compile(ParseContext* context);
	};
} // namespace lang