#if WITH_WINAPI
#include <modules/system.win32.hpp>
#include <parser/types/stringType.hpp>
#include <Windows.h>

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace lang;
using namespace lang::modules::system;

static void win32_messageBox(InterpretContext* context)
{
	int32_t type = context->popValue<int32_t>();
	RuntimeStr caption = context->popRuntimeString();
	RuntimeStr text = context->popRuntimeString();
	ClassPtr<HWND> cls = context->popValue<RuntimeClass*>();

	HWND window = cls.classPtr ? *cls.get() : nullptr;

	MessageBoxA(window, text.ptr(), caption.ptr(), type);
}

static void win32_getConsoleWindow(InterpretContext* context)
{
	HWND wnd = GetConsoleWindow();

	if (!wnd)
	{
		context->pushValue(nullptr);
		return;
	}

	ClassRef<HWND> cls = RuntimeClass::allocateClass(sizeof(HWND), nullptr);
	*cls.get() = wnd;
	context->pushValue(cls);
}

NativeModule win32::createModule()
{
	NativeModule out;
	out.name = "system::win32";

	auto windowType = out.createClass<HWND>("HWND");
	auto strType = StringType::getInstance();

	out.addFunction(NativeFunction(
		{
			FunctionArgument(windowType->nullable, "wnd"), FunctionArgument(strType, "text"),
			FunctionArgument(strType, "caption"), FunctionArgument(IntType::getInstance(), "type") },
		nullptr,
		"messageBox", &win32_messageBox));

	out.addFunction(NativeFunction({}, windowType->nullable, "getConsoleWindow", &win32_getConsoleWindow));

	return out;
}

#endif