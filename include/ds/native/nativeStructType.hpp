#pragma once
#include <ds/parser/types/classType.hpp>

#define DS_CREATE_STRUCT(type) new ::ds::NativeStructType(sizeof(type), #type)
#define DS_STRUCT_MEMBER(structValue, nativeStruct, name, memberType) \
	structValue->members.emplace_back(#name, Size(offsetof(nativeStruct, name)), memberType);
#define DS_STRUCT_MEMBER_NAME(structValue, nativeStruct, name, scriptName, memberType) \
	structValue->members.emplace_back(#scriptName, Size(offsetof(nativeStruct, name)), memberType);

namespace ds
{

	class NativeStructType : public ClassType
	{
	public:
		NativeStructType(Size size, std::string name);

		bool allowEmpty = true;

		BytecodeBuffer compileUnref() override;
		BytecodeBuffer compileMove(ParsedScope* with) override;
		BytecodeBuffer compileEndMove(ParsedScope* with) override;

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;

		ClassType* asClass() override;

		void addConstructor(Function* newConstructor);

		std::vector<std::pair<Operator, Function*>> operators;
	};
} // namespace ds