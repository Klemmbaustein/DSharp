#pragma once
#include "classType.hpp"

namespace lang
{
	struct ParsedFile;

	class FunctionType : public ClassType
	{
	public:
		FunctionType(Type* returnType, std::vector<Type*> arguments);
		Type* returnType;
		std::vector<Type*> arguments;

		ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors,
			ParsedScope* with, Type* hintType) override;
		ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;
		ExpressionResult compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;

		static FunctionType* compileType(Token first, TokenLine& line, ErrorContext* errors,
			ParsedFile* file);

		static FunctionType* getInstance(Type* returnType, std::vector<Type*> arguments)
		{
			FunctionType*& instance = functionTypes[getFunctionTypeName(returnType, arguments)];
			if (!instance)
			{
				instance = new FunctionType(returnType, arguments);
			}
			return instance;
		}

		Type* baseType;

		static std::string getFunctionTypeName(Type* returnType, const std::vector<Type*>& arguments);

	private:
		static inline std::map<std::string, FunctionType*> functionTypes;
	};
} // namespace lang