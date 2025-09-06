#include <language.hpp>
using namespace lang;

InterpretContext* lang::LanguageContext::createInterpreter()
{
	return new InterpretContext(this);
}

ParseContext* lang::LanguageContext::createCompiler()
{
	return new ParseContext(this);
}

#ifdef WITH_LANGUAGE_SERVICE
LanguageService* lang::LanguageContext::startService()
{
	return new LanguageService(this);
}
#endif

void lang::LanguageContext::addNativeModule(const NativeModule& module)
{
	this->languageModules.insert({ module.name, new NativeModule(module) });
}
