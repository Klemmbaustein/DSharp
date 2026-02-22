#ifdef WITH_LANGUAGE_SERVICE
#pragma once
#include <functional>
#include <ds/parser/parser.hpp>
#include "scannedSymbols.hpp"

namespace ds
{
	struct LanguageContext;

	class ScannedFile
	{
	public:
		std::vector<ScannedFunction> functions;
		std::vector<ScannedVariable> variables;
		std::vector<Token> types;
		std::map<TypeId, std::string> accessibleTypes;
		std::map<Token, std::string> accessibleFunctions;
		std::map<Token, std::string> accessibleEnums;

		std::vector<ScannedScope> scopes;
	};

	// clang-format off
	enum class CompletionType
	{
		variable     = 0b0000001,
		member       = 0b0000010,
		function     = 0b0000100,
		method       = 0b0001000,
		type         = 0b0010000,
		keyword      = 0b0100000,
		enumValue    = 0b1000000,
		classMembers = 0b0001010,
		all = 0xff
	};
	// clang-format on

	struct AutoCompleteResult
	{
		std::string name;
		std::string completionModule;
		CompletionType type = CompletionType::variable;
	};

	class LanguageService
	{
	public:
		LanguageService(LanguageContext* context);

		std::vector<std::string> scopeKeywords = {
			"return",
			"if",
			"else",
			"for",
			"continue",
			"while",
			"break",
			"throw",
			"const",
			"var",
			"new",
			"fn",
			"await",
			"not",
			"and",
			"or"
		};
		std::vector<std::string> fileKeywords = {
			"class",
			"interface",
			"module",
			"fn",
			"async",
			"virtual",
			"override"
		};

		void addString(const std::string& content, std::string name);
		void updateFile(const std::string& content, std::string name);
		void removeFile(std::string name);

		ParsedClass* addClass(Token className, std::string moduleName,
			ds::TokenStream& body, std::string fileName, std::vector<std::vector<ds::Token>> superClasses);
		ParsedClass* updateClass(Token className, std::string moduleName,
			ds::TokenStream& body, std::string fileName, std::vector<std::vector<ds::Token>> superClasses);

		void commitChanges();

		std::vector<AutoCompleteResult> completeAt(ScannedFile* f, size_t character, size_t line,
			CompletionType type);

		ParseContext* parser = nullptr;
		std::map<std::string, ScannedFile> files;
		std::map<TypeId, ScannedType> types;

	private:
		static bool includes(CompletionType a, CompletionType includes)
		{
			return (int(a) & int(includes)) != 0;
		}

		bool hasChanges = false;
		LanguageContext* language = nullptr;

		std::vector<AutoCompleteResult> completeType(ScannedType* type, CompletionType options);

		std::vector<AutoCompleteResult> completeScopeContents(ScannedFile* f, size_t character,
			size_t line, CompletionType options);
	};
} // namespace ds
#endif