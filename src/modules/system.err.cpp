#include <modules/system.err.hpp>

using namespace lang;
using namespace lang::modules::system;

static void err_getStackTrace()
{

}

static void err_abort(InterpretContext* context)
{
	context->runtimePanic(RuntimeStr("Aborted"));
	abort();
}

NativeModule err::createModule()
{
	NativeModule out;
	out.name = "system::err";

	out.addFunction(NativeFunction({}, nullptr, "abort", &err_abort));

	return out;
}
