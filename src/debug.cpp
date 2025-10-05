#include <ds/debug.hpp>

using namespace ds;

DebugInfo::DebugInfo()
{
}

DebugInfo::~DebugInfo()
{
}

DebugSection* ds::DebugInfo::getSectionAt(uint32_t offset)
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
