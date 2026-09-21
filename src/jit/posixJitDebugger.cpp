#include <ds/jit/posixJitDebugger.hpp>
#if _POSIX_VERSION
#include <ds/debug/jitDebugState.hpp>
#include <csignal>
#include <ds/jit/justInTimeCode_x64.hpp>

using namespace ds::jit;

static thread_local JustInTimeRuntime* currentJitRuntime;
static thread_local void* lastBreakpoint;

static void debugBreakpointHandler(int code, siginfo_t* info, void* processorState)
{
	auto state = reinterpret_cast<ucontext_t*>(processorState);

	currentJitRuntime->lastStackPos = reinterpret_cast<void*>(state->uc_mcontext.gregs[REG_RBP]);

	ds::Pointer stackDiff = state->uc_mcontext.gregs[REG_R13] - ds::Pointer(&currentJitRuntime->variableStack);
	currentJitRuntime->variableStackPos = stackDiff;

	auto debugState = new ds::JitDebugState(currentJitRuntime, state->uc_mcontext.gregs[REG_RIP]);
	std::vector<ds::Pointer> callStack = debugState->callStack;
	bool shouldContinue = currentJitRuntime->runtime->onDebugBreak(currentJitRuntime, debugState);
	delete debugState;

	auto bp = reinterpret_cast<void*>(state->uc_mcontext.gregs[REG_RIP]);

	// Currently fails here because REG_RIP seems to have a value that's
	// completely different from where the breakpoint was placed
	if (currentJitRuntime->code->clearBreakpoint(bp))
	{
		lastBreakpoint = bp;
	}
}

ds::jit::PosixJitDebugger::PosixJitDebugger(JustInTimeRuntime* rt)
{
	// Compiler get's *really* confused about the difference between the struct and the function
	// Why do they share the same name in the first place??
	struct sigaction newAction;

	// And sa_sigaction is a macro on linux??
	newAction.sa_sigaction = debugBreakpointHandler;
	newAction.sa_flags = SA_SIGINFO;

	::sigaction(SIGTRAP, &newAction, nullptr);

	this->runtime = rt;
}

ds::jit::PosixJitDebugger::~PosixJitDebugger()
{
}

void ds::jit::PosixJitDebugger::makeActive()
{
	currentJitRuntime = runtime;
}
#endif