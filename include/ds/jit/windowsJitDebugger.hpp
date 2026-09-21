#ifdef _WIN32
#pragma once
#include <ds/jit/platformJitDebugger.hpp>
#include <ds/jit/justInTime.hpp>

namespace ds::jit
{
	class WindowsJitDebugger : public PlatformJitDebugger
	{
	public:
		WindowsJitDebugger(JustInTimeRuntime* rt);
		~WindowsJitDebugger();

		JustInTimeRuntime* runtime = nullptr;
		void* breakpointHandle = nullptr;

		// Inherited via PlatformJitDebugger
		void makeActive() override;
	};
}
#endif