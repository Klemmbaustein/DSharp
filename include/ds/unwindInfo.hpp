#pragma once
#include "languageTypes.hpp"
#include <vector>

namespace ds
{
	enum class UnwindOp : uint8_t
	{
		popBytes,
		pushBytes,
		popClass,
	};

	struct UnwindPart
	{
		UnwindOp op = UnwindOp::popBytes;
		uint16_t size = 0;
		BytecodeOffset start = 0;
		BytecodeOffset offset = 0;
	};

	struct UnwindSection
	{
		uint32_t offset = 0;
		std::vector<UnwindPart> parts;
	};

	struct UnwindInfo
	{
		std::vector<UnwindSection> sections;
		UnwindSection* getSectionAt(uint32_t offset);
	};
}