#include <modules/standardLibrary.hpp>
#include <modules/system.hpp>
#include <modules/system.io.hpp>

void lang::modules::registerStandardLibrary(LanguageContext* context)
{
	context->addNativeModule(system::createModule());
	context->addNativeModule(system::io::createModule());
}
