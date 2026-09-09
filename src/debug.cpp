#include <ds/debug.hpp>

using namespace ds;

DebugInfo::DebugInfo()
{
}

DebugInfo::~DebugInfo()
{
}

DebugSection* ds::DebugInfo::getSectionAt(Pointer offset)
{
	if (offset == UINTPTR_MAX)
	{
		return nullptr;
	}

	DebugSection* last = nullptr;

	for (auto& i : sections)
	{
		if (i.offset >= offset)
		{
			return last;
		}
		last = &i;
	}
	return last;
}

DebugLine* ds::DebugInfo::getLineAt(std::string file, uint32_t lineNumber)
{
	DebugSection* last = nullptr;

	for (auto& section : this->sections)
	{
		if (section.file != file)
		{
			continue;
		}

		DebugLine* foundLine = section.getLineAt(lineNumber);

		if (foundLine)
		{
			return foundLine;
		}
	}
	return nullptr;
}

DebugLine* ds::DebugSection::getLineAt(uint32_t lineNumber)
{
	DebugLine* last = nullptr;

	for (auto& i : this->lines)
	{
		if (i.lineNumber > lineNumber)
		{
			return last;
		}
		last = &i;
	}
	return last && last->lineNumber == lineNumber ? last : nullptr;
}

std::pair<DebugSection*, DebugLine*> ds::DebugInfo::getLineAt(Pointer offset)
{
	auto section = getSectionAt(offset);
	DebugLine* last = nullptr;

	for (auto& i : section->lines)
	{
		if (i.offset > offset)
		{
			return { section, last };
		}
		last = &i;
	}
	return { section, last };
}