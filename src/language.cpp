#include <ds/language.hpp>
using namespace ds;

LanguageRuntime* ds::LanguageContext::createRuntime(RuntimeOptions options)
{
	return new LanguageRuntime(this, options);
}

ParseContext* ds::LanguageContext::createCompiler(ParserOptions options)
{
	return new ParseContext(this, options);
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
