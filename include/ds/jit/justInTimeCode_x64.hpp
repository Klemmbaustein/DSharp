#pragma once
#include <csetjmp>
#include <asmjit/x86.h>
#include <ds/languageTypes.hpp>
#include <ds/jit/justInTime.hpp>
#include <ds/jit/platformJitDebugger.hpp>

namespace ds::jit
{
	using JitEntryFunction = void (*)(void*, JustInTimeRuntime*);

	class JustInTimeCode
	{
	public:
		asmjit::JitRuntime jit;
		asmjit::CodeHolder compiled;
		JitEntryFunction entry = nullptr;

		void run(Pointer at, JustInTimeRuntime* runtime);
		void resume(void* at, JustInTimeRuntime* runtime);

		uint8_t insertBreakpoint(void* at, uint8_t byte);
		void insertBreakpoint(void* at);
		void restoreBreakpoint(void* at);
		bool clearBreakpoint(void* at);
		void removeBreakpoint(void* at);

		constexpr static uint8_t INSTRUCTION_INT3 = 0xCC;

		void getUnwindData(void* atPtr, std::vector<Pointer>& outPointers, bool skipFirst = false);

		void unwindStack(void* atPtr, JustInTimeRuntime* rt);
		void doUnwind(std::vector<Pointer> callAddresses, JustInTimeRuntime* rt, bool isSuspended);

		void initializeBreakpointHandler(JustInTimeRuntime* rt);

	private:
		std::map<Pointer, uint8_t> debugReplacedBytes;
		PlatformJitDebugger* debugger = nullptr;
		bool unwinding = false;
		jmp_buf returnBuffer{};
	};
} // namespace ds::jit