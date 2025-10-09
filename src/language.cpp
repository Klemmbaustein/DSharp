#include <ds/language.hpp>
using namespace ds;

LanguageRuntime* ds::LanguageContext::createRuntime()
{
	return new LanguageRuntime(this);
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
