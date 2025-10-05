#include <ds/modules/system.err.hpp>

using namespace ds;
using namespace ds::modules::system;

static void err_getStackTrace()
{

}

static void err_abort(InterpretContext* context)
{
	context->runtimePanic(RuntimeStr("Aborted"));
}

NativeModule err::createModule()
{
	NativeModule out;
	out.name = "system::err";

	out.addFunction(NativeFunction({}, nullptr, "abort", &err_abort));

	return out;
}
