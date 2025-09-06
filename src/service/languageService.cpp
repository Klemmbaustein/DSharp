#ifdef WITH_LANGUAGE_SERVICE
#include <service/languageService.hpp>

lang::LanguageService::LanguageService(LanguageContext* context)
{
	this->parser = new ParseContext(context);
	this->parser->service = this;
}

void lang::LanguageService::addString(const std::string& content, std::string name)
{
	this->parser->addString(content, name);
}

void lang::LanguageService::updateFile(const std::string& str, std::string fileName)
{
	this->parser->updateFile(str, fileName);
}

void lang::LanguageService::commitChanges()
{
	this->parser->errors.reset();
	this->parser->compile();
}

#endif
