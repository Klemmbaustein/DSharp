#include <ds/modules/standardLibrary.hpp>
#include <ds/modules/system.hpp>
#include <ds/modules/system.async.hpp>
#include <ds/modules/system.io.hpp>
#include <ds/modules/system.fs.hpp>
#include <ds/modules/system.err.hpp>
#include <ds/modules/system.win32.hpp>

void ds::modules::registerStandardLibrary(LanguageContext* context)
{
	context->addNativeModule(system::createModule());
	context->addNativeModule(system::async::createModule());
	context->addNativeModule(system::io::createModule());
#if MODULE_FS
	context->addNativeModule(system::fs::createModule());
#endif
	context->addNativeModule(system::err::createModule());
#if WITH_WINAPI
	context->addNativeModule(system::win32::createModule());
	#endif
}
