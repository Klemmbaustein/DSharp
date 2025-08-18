#if WITH_WINAPI
#include <modules/system.win32.hpp>
#include <parser/types/stringType.hpp>
#include <Windows.h>
#include <print>

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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam); // add this
	}
	return 0;
}

static void win32_createWindow(InterpretContext* context)
{
	RuntimeClass* param = context->popValue<RuntimeClass*>();
	ClassPtr<HINSTANCE> instanceHandle = context->popValue<RuntimeClass*>();
	ClassPtr<HMENU> menuHandle = context->popValue<RuntimeClass*>();
	ClassPtr<HWND> parent = context->popValue<RuntimeClass*>();
	int height = context->popValue<int32_t>();
	int width = context->popValue<int32_t>();
	int y = context->popValue<int32_t>();
	int x = context->popValue<int32_t>();
	DWORD style = context->popValue<int32_t>();
	RuntimeStr title = context->popRuntimeString();
	RuntimeStr className = context->popRuntimeString();
	DWORD exStyle = context->popValue<int32_t>();

	const char CLASS_NAME[] = "Sample Window Class";

	WNDCLASS wc = { 0 };

	wc.lpfnWndProc = &WindowProc;
	wc.lpszClassName = CLASS_NAME;
	wc.hInstance = GetModuleHandle(NULL);
	RegisterClass(&wc);

	auto wnd = CreateWindowExA(exStyle, CLASS_NAME, title.ptr(), style, x, y, width, height, nullptr, nullptr, GetModuleHandle(NULL), nullptr);

	if (!wnd)
	{
		context->pushValue(nullptr);
		return;
	}

	ShowWindow(wnd, SW_SHOW);

	ClassRef<HWND> cls = RuntimeClass::allocateClass(sizeof(HWND), nullptr);
	*cls.get() = wnd;
	context->pushValue(cls);
}

NativeModule win32::createModule()
{
	NativeModule out;
	out.name = "system::win32";

	auto handleType = out.createClass<HANDLE>("HANDLE");
	auto windowType = out.createClass<HWND>("HWND", handleType);
	auto instanceType = out.createClass<HINSTANCE>("HINSTANCE", handleType);
	auto strType = StringType::getInstance();
	auto intType = IntType::getInstance();

	out.addFunction(NativeFunction(
		{ FunctionArgument(windowType->nullable, "wnd"), FunctionArgument(strType, "text"),
			FunctionArgument(strType, "caption"), FunctionArgument(intType, "type") },
		nullptr,
		"messageBox", &win32_messageBox));

	auto IdEnum = out.createEnum("CommandID");
	out.addEnumValue(IdEnum, "ok", IDOK);
	out.addEnumValue(IdEnum, "cancel", IDCANCEL);
	out.addEnumValue(IdEnum, "abort", IDABORT);
	out.addEnumValue(IdEnum, "retry", IDRETRY);
	out.addEnumValue(IdEnum, "ignore", IDIGNORE);
	out.addEnumValue(IdEnum, "yes", IDYES);
	out.addEnumValue(IdEnum, "no", IDNO);
	out.addEnumValue(IdEnum, "tryAgain", IDTRYAGAIN);
	out.addEnumValue(IdEnum, "continue", IDCONTINUE);

	auto messageBoxEnum = out.createEnum("MBType");
	out.addEnumValue(messageBoxEnum, "abortRetryIgnore", MB_ABORTRETRYIGNORE);
	out.addEnumValue(messageBoxEnum, "cancelTryContinue", MB_CANCELTRYCONTINUE);
	out.addEnumValue(messageBoxEnum, "help", MB_HELP);
	out.addEnumValue(messageBoxEnum, "ok", MB_OK);
	out.addEnumValue(messageBoxEnum, "okCancel", MB_OKCANCEL);
	out.addEnumValue(messageBoxEnum, "retryCancel", MB_RETRYCANCEL);
	out.addEnumValue(messageBoxEnum, "yesNo", MB_YESNO);
	out.addEnumValue(messageBoxEnum, "yesNoCancel", MB_YESNOCANCEL);

	out.addEnumValue(messageBoxEnum, "iconExclamation", MB_ICONEXCLAMATION);
	out.addEnumValue(messageBoxEnum, "iconWarning", MB_ICONWARNING);
	out.addEnumValue(messageBoxEnum, "iconInformation", MB_ICONINFORMATION);
	out.addEnumValue(messageBoxEnum, "iconAsterisk", MB_ICONASTERISK);
	out.addEnumValue(messageBoxEnum, "iconQuestion", MB_ICONQUESTION);
	out.addEnumValue(messageBoxEnum, "iconStop", MB_ICONSTOP);
	out.addEnumValue(messageBoxEnum, "iconError", MB_ICONERROR);
	out.addEnumValue(messageBoxEnum, "iconHand", MB_ICONHAND);

	auto windowStyleEnum = out.createEnum("WindowStyle");
	out.addEnumValue(windowStyleEnum, "overlappedWindow", WS_OVERLAPPEDWINDOW);
	out.addEnumValue(windowStyleEnum, "overlapped", WS_OVERLAPPED);

	out.addFunction(NativeFunction({}, windowType->nullable, "getConsoleWindow", &win32_getConsoleWindow));
	out.addFunction(NativeFunction(
		{
			FunctionArgument(intType, "exStyle"),
			FunctionArgument(strType->nullable, "className"),
			FunctionArgument(strType->nullable, "windowName"),
			FunctionArgument(intType, "style"),
			FunctionArgument(intType, "x"),
			FunctionArgument(intType, "y"),
			FunctionArgument(intType, "width"),
			FunctionArgument(intType, "height"),
			FunctionArgument(windowType->nullable, "windowParent"),
			FunctionArgument(windowType->nullable, "menu"),
			FunctionArgument(strType->nullable, "instance"),
			FunctionArgument(strType->nullable, "param"),
		},
		windowType->nullable, "createWindowEx", &win32_createWindow));

	return out;
}

#endif