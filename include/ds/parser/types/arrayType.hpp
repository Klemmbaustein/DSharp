#pragma once
#include "classType.hpp"
#include <map>

namespace ds
{
	struct ParseContext;

	class ArrayType : public ClassType
	{
	public:
		ArrayType(Type* baseType, TypeRegistry* registry);
		~ArrayType();

		ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors,
			ParsedScope* with, Type* hintType) override;
		ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		ExpressionResult compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;

		ExpressionResult getLength(BytecodeBuffer thisValue, ParseContext* context);

		ExpressionResult makeArrayValue(std::vector<ExpressionResult> values, ParsedScope* with);
		void applyName() override;

		virtual std::vector<Type*> getGenericTypes()
		{
			return {baseType};
		}

		static constexpr inline TypeId ARRAY_BASE_TYPE = typeIdFromName("<array>");

		Type* baseType;
	};
}
