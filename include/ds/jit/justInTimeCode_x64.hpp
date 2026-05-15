#pragma once
#include <csetjmp>
#include <asmjit/x86.h>
#include <unordered_map>
#include <ds/languageTypes.hpp>
#include "justInTime.hpp"

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

		void getUnwindData(void* atPtr, std::vector<Pointer>& outPointers);

		[[noreturn]]
		void unwindStack(void* atPtr, JustInTimeRuntime* rt);

	private:
		bool unwinding = false;
		jmp_buf returnBuffer{};
	};
} // namespace ds::jit