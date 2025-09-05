#pragma once
#include <cstdint>

namespace lang
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
	using Pointer = size_t;
	using Size = uint32_t;
	using bytecodeOffset = uint32_t;

	class InterpretContext;

	struct VTableEntry
	{
		bytecodeOffset codeOffset = UINT32_MAX;
		void (*nativeFn)(InterpretContext*) = nullptr;

		operator bool()
		{
			return codeOffset != UINT32_MAX || bool(nativeFn);
		}
	};

} // namespace lang