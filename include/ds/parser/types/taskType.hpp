#pragma once
#include "classType.hpp"
#include <map>

namespace ds
{
	class TaskType : public ClassType
	{
	public:
		TaskType(Type* baseType, TypeRegistry* registry);

		ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;

		ExpressionResult compileAwait(ExpressionResult taskExpr, ExpressionResult returnTaskExpr,
			ParsedScope* with);

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

		static TaskType* getInstance(Type* baseType, TypeRegistry* registry)
		{
			return registry->getClassGenericEntry<TaskType>({ baseType }, [&] {
				return new TaskType(baseType, registry);
			});
		}
	};
} // namespace ds
