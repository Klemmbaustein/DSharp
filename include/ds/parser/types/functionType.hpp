#pragma once
#include "classType.hpp"

namespace ds
{
	struct ParsedFile;

	class FunctionType : public ClassType
	{
	public:
		FunctionType(Type* returnType, std::vector<Type*> arguments);
		~FunctionType();
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

		std::vector<Type*> getGenericTypes() override
		{
			std::vector outTypes = arguments;
			outTypes.push_back(returnType);
			return outTypes;
		}

		static FunctionType* getInstance(Type* returnType, std::vector<Type*> arguments, TypeRegistry* registry)
		{
			std::vector inArgs = arguments;
			inArgs.push_back(returnType);
			return registry->getClassGenericEntry<FunctionType>(inArgs, [&] {
				return new FunctionType(returnType, arguments);
			});
		}

		Type* instantiateGeneric(std::vector<Type*> types, Token at, ErrorContext* with, TypeRegistry* registry) override
		{
			auto lastArg = types[types.size() - 1];
			types.pop_back();
			return getInstance(lastArg, types, registry);
		}

		Type* baseType;

		static std::string getFunctionTypeName(Type* returnType, const std::vector<Type*>& arguments);
	};
} // namespace ds