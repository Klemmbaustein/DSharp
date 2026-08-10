#pragma once
#include <string>
#include <map>
#include <vector>
#include "class.hpp"
#include "typeId.hpp"
#include <optional>

namespace ds
{
	struct AttributeData
	{
		TypeId type = 0;
		std::vector<std::string> parameterData;

		std::optional<std::string> getParameterValue(const std::string& name) const;
	};

	struct TypeMember
	{
		TypeId type = 0;
		std::string name;
		AttributeData attribute;

		BytecodeOffset offset = 0;
	};

	struct TypeInfo
	{
		TypeId hash = 0;
		std::string name;
		Size vTableOffset = 0;
		Pointer constructor = 0;
		size_t bodySize = 0;

		std::vector<AttributeData> attributes;

		std::vector<TypeMember> members;
		TypeId superClass = 0;
		std::map<TypeId, BytecodeOffset> interfaces;

		RuntimeClass* create(InterpretContext* context) const;
	};

	struct CastResult
	{
		bool success = false;
		bool isInterface = false;
		Int interfaceOffset = 0;
	};

	struct ReflectInfo
	{
		bool isSubclassOf(TypeId toCheck, TypeId superClass) const;
		CastResult tryCast(TypeId toCheck, TypeId superClass) const;

		std::map<TypeId, TypeInfo> types;
	};
}