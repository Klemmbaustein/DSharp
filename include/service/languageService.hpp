#ifdef WITH_LANGUAGE_SERVICE
#pragma once
#include <functional>
#include <parser/parser.hpp>

namespace lang
{
	struct LanguageContext;

	class ScannedFile
	{
	public:
		std::vector<Token> functions;
		std::vector<Token> variables;
		std::vector<Token> types;
	};

	class LanguageService
	{
	public:

		LanguageService(LanguageContext* context);

		void addString(const std::string& content, std::string name);
		void updateFile(const std::string& content, std::string name);

		void commitChanges();

		ParseContext* parser = nullptr;
		std::map<std::string, ScannedFile> files;
	};
}
#endif