#pragma once
#include <string>
#include <map>
#include <vector>
#include "class.hpp"
#include "typeId.hpp"

namespace ds
{
	struct TypeMember
	{
		TypeId type = 0;
		std::string name;
		bytecodeOffset offset = 0;
	};

	struct TypeInfo
	{
		TypeId hash = 0;
		std::string name;
		Size vTableOffset = 0;
		bytecodeOffset constructor = 0;
		size_t bodySize = 0;

		std::vector<TypeMember> members;
		std::vector<TypeId> superClasses;

		RuntimeClass* create(InterpretContext* context) const;
	};

	struct ReflectInfo
	{
		bool isSubclassOf(TypeId toCheck, TypeId superClass) const;

		std::map<TypeId, TypeInfo> types;
	};
}