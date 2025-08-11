#include <debug.hpp>

using namespace lang;

DebugInfo::DebugInfo()
{
}

DebugInfo::~DebugInfo()
{
}

DebugSection* lang::DebugInfo::getSectionAt(uint32_t offset)
{
	DebugSection* last = nullptr;

	for (auto& i : sections)
	{
		if (i.offset > offset)
		{
			return last;
		}
		last = &i;
	}
	return last;
}
