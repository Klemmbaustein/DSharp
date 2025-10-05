#pragma once
#include <string>
#include <map>
#include <vector>
#include "class.hpp"

namespace ds
{
	using TypeHash = uint32_t;

	struct TypeMember
	{
		TypeHash type = 0;
		std::string name;
		bytecodeOffset offset = 0;
	};

	struct TypeInfo
	{
		TypeHash hash = 0;
		std::string name;
		Size vTableOffset = 0;
		bytecodeOffset constructor = 0;
		size_t bodySize = 0;

		std::vector<TypeMember> members;

		RuntimeClass* create(InterpretContext* context) const;
	};

	struct ReflectInfo
	{
		std::map<TypeHash, TypeInfo> types;
	};
}