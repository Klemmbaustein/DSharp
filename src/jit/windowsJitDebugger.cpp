#ifdef _WIN64
#include <ds/jit/windowsJitDebugger.hpp>
#include <ds/jit/justInTimeCode_x64.hpp>
#include <Windows.h>
#include <ds/debug/jitDebugState.hpp>
#include <errhandlingapi.h>

using namespace ds::jit;

static thread_local JustInTimeRuntime* currentJitRuntime;
static thread_local void* lastBreakpoint;

static void JitBreakpointDoUnwind(const std::vector<ds::Pointer>& unwindAfter)
{
	currentJitRuntime->code->doUnwind(unwindAfter, currentJitRuntime, false);
}

static LONG JitBreakpointException(_In_ _EXCEPTION_POINTERS* ExceptInfo)
{
	if (ExceptInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
	{
		currentJitRuntime->code->restoreBreakpoint(lastBreakpoint);
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	if (ExceptInfo->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	currentJitRuntime->lastStackPos = reinterpret_cast<void*>(ExceptInfo->ContextRecord->Rbp);

	ds::Pointer stackDiff = ExceptInfo->ContextRecord->R13 - ds::Pointer(&currentJitRuntime->variableStack);
	currentJitRuntime->variableStackPos = stackDiff;

	auto state = new ds::JitDebugState(currentJitRuntime, ExceptInfo->ContextRecord->Rip);
	std::vector<ds::Pointer> callStack = state->callStack;
	bool shouldContinue = currentJitRuntime->runtime->onDebugBreak(currentJitRuntime, state);
	delete state;

	if (!shouldContinue)
	{
		JitBreakpointDoUnwind(callStack);
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	auto bp = reinterpret_cast<void*>(ExceptInfo->ContextRecord->Rip);
	if (currentJitRuntime->code->clearBreakpoint(bp))
	{
		lastBreakpoint = bp;
		// Set x86 trap flag so the breakpoint can be re-inserted on the next instruction
		ExceptInfo->ContextRecord->EFlags |= 0x0100;
	}

	return EXCEPTION_CONTINUE_EXECUTION;
}

ds::jit::WindowsJitDebugger::WindowsJitDebugger(JustInTimeRuntime* rt)
{
	this->runtime = rt;

	currentJitRuntime = rt;

	breakpointHandle = AddVectoredExceptionHandler(0, JitBreakpointException);
}

ds::jit::WindowsJitDebugger::~WindowsJitDebugger()
{
	if (breakpointHandle)
	{
		RemoveVectoredExceptionHandler(breakpointHandle);
	}
}

void ds::jit::WindowsJitDebugger::makeActive()
{
	currentJitRuntime = runtime;
}
#endif