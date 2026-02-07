#include <ds/modules/standardLibrary.hpp>
#include <ds/modules/system.hpp>
#include <ds/modules/system.async.hpp>
#include <ds/modules/system.io.hpp>
#include <ds/modules/system.fs.hpp>
#include <ds/modules/system.math.hpp>
#include <ds/modules/system.err.hpp>

void ds::modules::registerStandardLibrary(LanguageContext* context)
{
	context->addNativeModule(system::createModule(context));
	context->addNativeModule(system::async::createModule(context));
	context->addNativeModule(system::io::createModule(context));
	context->addNativeModule(system::err::createModule());
	context->addNativeModule(system::math::createModule(context));

#if MODULE_FS
	context->addNativeModule(system::fs::createModule(context));
#endif
}
