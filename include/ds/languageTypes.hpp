#pragma once
#include <cstdint>
#include <cstddef>

namespace ds
{
#if LANG_ALIGN_TYPES_32BIT
	using Bool = int32_t;
	using Char = int32_t;
#else
	using Bool = bool;
	using Char = char;
#endif

	using Int = int32_t;
	using Float = float;
	using Pointer = uintptr_t;
	using Size = uint32_t;
	using BytecodeOffset = uint32_t;
	using TypeId = Size;

	class InterpretContext;

	struct RuntimeFunction
	{
		Pointer codeOffset = UINTPTR_MAX;
		void (*nativeFn)(InterpretContext*) = nullptr;

		operator bool()
		{
			return (codeOffset != UINTPTR_MAX) || bool(nativeFn);
		}
	};

} // namespace ds