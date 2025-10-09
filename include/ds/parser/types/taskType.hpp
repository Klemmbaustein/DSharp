#pragma once
#include "classType.hpp"
#include <map>

namespace ds
{
	class TaskType : public ClassType
	{
	public:
		TaskType(Type* baseType);

		ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;

		static TaskType* getInstance(Type* baseType)
		{
			TaskType*& instance = taskTypes[baseType];
			if (!instance)
			{
				instance = new TaskType(baseType);
			}
			return instance;
		}

		ExpressionResult compileAwait(ExpressionResult taskExpr, ExpressionResult returnTaskExpr,
			ParsedScope* with);

		ExpressionResult compileTask();
		ExpressionResult compileCompleteTask();

		Type* baseType;

	private:
		static inline std::map<Type*, TaskType*> taskTypes;
	};
} // namespace ds
