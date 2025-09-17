#ifdef WITH_LANGUAGE_SERVICE
#pragma once
#include <functional>
#include <parser/parser.hpp>

namespace lang
{
	struct LanguageContext;

	struct ScannedFunction
	{
		Token at;
		std::string name;
		std::optional<FunctionDefinition> definition;
		std::string returnType;
		std::vector<std::pair<std::string, std::string>> arguments;

		ScannedFunction(Function* from, Token atToken)
		{
			this->at = atToken;
			this->name = from->getFullName();
			auto functionArgs = from->getArguments();
			this->arguments.reserve(functionArgs.size());

			for (auto& i : functionArgs)
			{
				this->arguments.push_back({ i.type->getName(), i.name.string });
			}

			auto functionReturnType = from->getReturnType();

			definition = from->getDefinition();

			if (functionReturnType)
			{
				this->returnType = functionReturnType->getName();
			}
		}
	};

	class ScannedFile
	{
	public:
		std::vector<ScannedFunction> functions;
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