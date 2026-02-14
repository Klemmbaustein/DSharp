#pragma once
#include "languageTypes.hpp"
#include <string>

namespace ds
{
	/// Unique (usually hashed from the name) identifier for a type.
	using TypeId = Size;

	constexpr TypeId typeIdFromName(std::string name)
	{
		TypeId hash = 1315423911;

		for (char c : name)
		{
			hash ^= ((hash << 5) + c + (hash >> 2));
		}

		return (hash & 0x7FFFFFFF);
	}

}