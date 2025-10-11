#pragma once
#include <vector>
#include <string>
#include <ds/typeId.hpp>

namespace ds
{
	class ScannedMember
	{
	public:
		TypeId memberTypeId;
		std::string memberTypeName;
		std::string name;
	};

	class ScannedType
	{
	public:
		std::string name;
		TypeId id = 0;

		std::vector<ScannedMember> members;
	};
}