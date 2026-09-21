#pragma once
#include <exception>

namespace ds::jit
{
	class JitDebugNotSupportedException : std::exception
	{
	public:

		const char* what() const noexcept final
		{
			return "Debugger not supported!";
		}
	};

	class PlatformJitDebugger
	{
	public:
		virtual ~PlatformJitDebugger() = default;

		virtual void makeActive() = 0;
	};
}