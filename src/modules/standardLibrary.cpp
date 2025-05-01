#include <modules/standardLibrary.hpp>
#include <modules/system.hpp>

void lang::modules::registerStandardLibrary(LanguageContext* context)
{
	context->addNativeModule(system::createModule());
}
