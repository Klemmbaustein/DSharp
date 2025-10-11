#include <ds/typeId.hpp>

using namespace ds;

TypeId ds::typeIdFromName(std::string name)
{
	TypeId hash = 1315423911;

	for (char c : name)
	{
		hash ^= ((hash << 5) + c + (hash >> 2));
	}

	return (hash & 0x7FFFFFFF);
}
