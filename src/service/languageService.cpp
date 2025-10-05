#ifdef WITH_LANGUAGE_SERVICE
#include <ds/service/languageService.hpp>

ds::LanguageService::LanguageService(LanguageContext* context)
{
	this->parser = new ParseContext(context);
	this->parser->service = this;
}

void ds::LanguageService::addString(const std::string& content, std::string name)
{
	this->parser->addString(content, name);
}

void ds::LanguageService::updateFile(const std::string& str, std::string fileName)
{
	this->parser->updateFile(str, fileName);
}

void ds::LanguageService::commitChanges()
{
	this->parser->errors.reset();
	this->parser->compile();
}

#endif
