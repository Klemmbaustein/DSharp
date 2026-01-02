#pragma once
#include <ds/parser/types/classType.hpp>

#define DS_CREATE_STRUCT(type) new ::ds::NativeStructType(sizeof(type), # type)
#define DS_STRUCT_MEMBER(x, type, name, memberType) x->members.emplace_back(# name, Size(offsetof(type, name)), memberType);
#define DS_STRUCT_MEMBER_NAME(x, type, name, scriptName, memberType) x->members.emplace_back(#scriptName, Size(offsetof(type, name)), memberType);

namespace ds
{

	class NativeStructType : public ClassType
	{
	public:
		NativeStructType(Size size, std::string name);

		BytecodeBuffer compileUnref() override;
		BytecodeBuffer compileMove(ParsedScope* with) override;
		BytecodeBuffer compileEndMove(ParsedScope* with) override;

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;;

		void addConstructor(Function* newConstructor);

		std::vector<std::pair<Operator, Function*>> operators;
	};
} // namespace ds