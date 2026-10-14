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

		template <typename T>
		T getFromInstance(RuntimeClass* fromInstance) const
		{
			T result{};
			memcpy(&result, fromInstance->getBasePtr()->getBody() + offset, sizeof(T));

			return result;
		}

		template <typename T>
		void setToInstance(RuntimeClass* toInstance, const T& value) const
		{
			memcpy(toInstance->getBasePtr()->getBody() + offset, &value, sizeof(T));
		}

		/**
		 * @brief
		 * Sets this type member of an instance to a class value, and properly de-references
		 * the old  value.
		 * @param toInstance
		 * The class instance to write to.
		 * @param value
		 * The new value to set.
		 * @param context
		 * The interpret context that should de-reference the old value.
		 * If the old value is guaranteed to be null, this value can be null.
		 */
		void setClassToInstance(RuntimeClass* toInstance, RuntimeClass* value, InterpretContext* context) const;
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

	struct EntryPoint
	{
		std::string name;
		AttributeData attribute;
		TypeId returnValue;
		std::vector<TypeId> arguments;
		Pointer codePosition = 0;
	};

	struct ReflectInfo
	{
		bool isSubclassOf(TypeId toCheck, TypeId superClass) const;
		CastResult tryCast(TypeId toCheck, TypeId superClass) const;

		std::vector<EntryPoint> entryPoints;
		std::map<TypeId, TypeInfo> types;
	};
} // namespace ds