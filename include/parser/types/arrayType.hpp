#pragma once
#include "classType.hpp"
#include <map>

namespace lang
{
	class ArrayType : public ClassType
	{
	public:
		ArrayType(Type* baseType);

		ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors,
			ParsedScope* with, Type* hintType) override;
		ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;
		ExpressionResult compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;

		static ArrayType* getInstance(Type* baseType)
		{
			ArrayType*& instance = arrayTypes[baseType];
			if (!instance)
			{
				instance = new ArrayType(baseType);
			}
			return instance;
		}

		ExpressionResult makeArrayValue(std::vector<ExpressionResult> values, ParsedScope* with);

		Type* baseType;

	private:
		static inline std::map<Type*, ArrayType*> arrayTypes;
	};
}
