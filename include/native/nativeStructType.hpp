#pragma once
#include <parser/types/type.hpp>

#define LANG_CREATE_STRUCT(type) new ::lang::NativeStructType(sizeof(type), # type)
#define LANG_STRUCT_MEMBER(x, type, name, memberType) x->members.emplace_back(# name, offsetof(type, name), memberType);
#define LANG_STRUCT_MEMBER_NAME(x, type, name, scriptName, memberType) x->members.emplace_back(#scriptName, offsetof(type, name), memberType);

namespace lang
{
	struct NativeStructMember
	{
		std::string name;
		size_t offset = 0;
		Type* memberType = nullptr;
	};

	class NativeStructType : public Type
	{
	public:
		NativeStructType(Size size, std::string name);

		std::string getName() override;

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;;

		std::vector<NativeStructMember> members;
		std::map<std::string, Function*> methods;
		std::vector<Function*> constructors;
		std::vector<std::pair<Operator, Function*>> operators;
	};
} // namespace lang