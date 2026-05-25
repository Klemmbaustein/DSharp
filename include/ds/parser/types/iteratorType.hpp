#pragma once
#include "classType.hpp"

namespace ds
{
	class IteratorType : public ClassType
	{
	public:
		IteratorType(Type* baseType, TypeRegistry* registry);

		ExpressionResult compileAwait(ExpressionResult taskExpr, ExpressionResult returnTaskExpr,
			ParsedScope* with) const;

		ExpressionResult compileTask();
		ExpressionResult compileCompleteTask();

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

		Type* instantiateGeneric(std::vector<Type*> types, Token at, ErrorContext* with, TypeRegistry* registry) override
		{
			return getInstance(types[0], registry);
		}

		static IteratorType* getInstance(Type* baseType, TypeRegistry* registry)
		{
			return registry->getClassGenericEntry<IteratorType>({ baseType }, [&] {
				return new IteratorType(baseType, registry);
			});
		}
	};
} // namespace ds