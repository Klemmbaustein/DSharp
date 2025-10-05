#include <ds/language.hpp>
using namespace ds;

InterpretContext* ds::LanguageContext::createInterpreter()
{
	return new InterpretContext(this);
}

ParseContext* ds::LanguageContext::createCompiler()
{
	return new ParseContext(this);
}

#ifdef WITH_LANGUAGE_SERVICE
LanguageService* ds::LanguageContext::startService()
{
	return new LanguageService(this);
}
#endif

void ds::LanguageContext::addNativeModule(const NativeModule& module)
{
	auto m = new NativeModule(module);
	m->initialize();
	this->languageModules.insert({ module.name, m });
}
