#include <modules/standardLibrary.hpp>
#include <modules/system.hpp>
#include <modules/system.io.hpp>
#include <modules/system.fs.hpp>

void lang::modules::registerStandardLibrary(LanguageContext* context)
{
	context->addNativeModule(system::createModule());
	context->addNativeModule(system::io::createModule());
	context->addNativeModule(system::fs::createModule());
}
