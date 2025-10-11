#pragma once
#include "languageTypes.hpp"
#include <string>

namespace ds
{
	using TypeId = Size;

	TypeId typeIdFromName(std::string name);
}