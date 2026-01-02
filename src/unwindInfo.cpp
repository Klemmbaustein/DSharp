#include <ds/unwindInfo.hpp>

using namespace ds;

UnwindSection* ds::UnwindInfo::getSectionAt(uint32_t offset)
{
	if (offset == SIZE_MAX)
	{
		return nullptr;
	}

	UnwindSection* last = nullptr;

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