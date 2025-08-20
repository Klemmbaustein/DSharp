#include <modules/standardLibrary.hpp>
#include <modules/system.hpp>
#include <modules/system.io.hpp>
#include <modules/system.fs.hpp>
#include <modules/system.err.hpp>
#include <modules/system.win32.hpp>

void lang::modules::registerStandardLibrary(LanguageContext* context)
{
	context->addNativeModule(system::createModule());
	context->addNativeModule(system::io::createModule());
#if MODULE_FS
	context->addNativeModule(system::fs::createModule());
#endif
	context->addNativeModule(system::err::createModule());
#if WITH_WINAPI
	context->addNativeModule(system::win32::createModule());
	#endif
}
