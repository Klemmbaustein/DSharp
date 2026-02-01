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

		std::vector<ScannedScope> scopes;
	};

	enum class CompletionType
	{
		variable     = 0b000001,
		member       = 0b000010,
		function     = 0b000100,
		method       = 0b001000,
		type         = 0b010000,
		keyword      = 0b100000,
		classMembers = 0b001010,
		all = 0xff
	};

	struct AutoCompleteResult
	{
		std::string name;
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