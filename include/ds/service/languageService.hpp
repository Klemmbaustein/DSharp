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

	struct AutoCompleteResult
	{
		std::string name;
	};

	enum class CompletionType
	{
		member,
		variables,
		all,
	};

	class LanguageService
	{
	public:
		LanguageService(LanguageContext* context);

		void addString(const std::string& content, std::string name);
		void updateFile(const std::string& content, std::string name);

		void commitChanges();

		std::vector<AutoCompleteResult> completeAt(ScannedFile* f, size_t character, size_t line,
			CompletionType type);

		ParseContext* parser = nullptr;
		std::map<std::string, ScannedFile> files;
		std::map<TypeId, ScannedType> types;

	private:
		std::vector<AutoCompleteResult> completeScopeContents(ScannedFile* f, size_t character, size_t line);
	};
} // namespace ds
#endif