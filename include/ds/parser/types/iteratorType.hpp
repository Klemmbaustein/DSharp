#pragma once
#include "classType.hpp"

namespace ds
{
	class IteratorType : public ClassType
	{
	public:
		IteratorType(Type* baseType, TypeRegistry* registry);
		~IteratorType();

		Type* baseType;

		std::vector<GenericArgument> getGenericArguments() override
		{
			return { GenericArgument(Token("T"), nullptr) };
		}

		std::vector<Type*> getGenericTypes() override
		{
			if (baseType)
				return { baseType };
			return {};
		}

		BytecodeBuffer compileNext(BytecodeBuffer thisValue, ErrorContext* errors, ParsedScope* with);
		ExpressionResult compileGet(BytecodeBuffer thisValue, ErrorContext* errors, ParsedScope* with);

		Type* instantiateGeneric(std::vector<Type*> types, Token at, ErrorContext* with, TypeRegistry* registry) override
		{
			return getInstance(types[0], registry);
		}

		static IteratorType* getInstance(Type* baseType, TypeRegistry* registry)
		{
			return registry->getClassGenericEntry<IteratorType>({ baseType }, [baseType, registry] {
				return new IteratorType(baseType, registry);
			});
		}
	};
} // namespace ds