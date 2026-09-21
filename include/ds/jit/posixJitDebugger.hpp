#if _POSIX_VERSION
#pragma once
#include <ds/jit/platformJitDebugger.hpp>
#include <ds/jit/justInTime.hpp>

namespace ds::jit
{
	class PosixJitDebugger : public PlatformJitDebugger
	{
	public:
		PosixJitDebugger(JustInTimeRuntime* rt);
		~PosixJitDebugger();

		// Inherited via PlatformJitDebugger
		void makeActive() override;
		JustInTimeRuntime* runtime = nullptr;
	};
}
#endif