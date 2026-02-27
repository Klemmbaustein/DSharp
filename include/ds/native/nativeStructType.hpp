#pragma once
#include <ds/parser/types/classType.hpp>

// Some utility macros.
#define DS_CREATE_STRUCT(type) new ::ds::NativeStructType(sizeof(type), #type)
#define DS_STRUCT_MEMBER(structValue, nativeStruct, name, memberType) \
	structValue->members.emplace_back(#name, Size(offsetof(nativeStruct, name)), memberType);
#define DS_STRUCT_MEMBER_NAME(structValue, nativeStruct, name, scriptName, memberType) \
	structValue->members.emplace_back(#scriptName, Size(offsetof(nativeStruct, name)), memberType);

namespace ds
{
	/**
	 * @brief
	 * A class representing a native data structure as a pass-by-value object.
	 */
	class NativeStructType : public ClassType
	{
	public:
		NativeStructType(Size size, std::string name);

		/**
		 * @brief
		 * Allow the struct to be initialized as empty,
		 * leaving all it's members at 0 without calling a constructor.
		 */
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

		/**
		 * @brief
		 * Adds a constructor to this struct
		 * @param newConstructor
		 * The new constructor to add.
		 */
		void addConstructor(Function* newConstructor);

		/**
		 * @brief
		 * List of operator functions that will be called when an operator is used.
		 * For example s1 + s2 would be handled by the first function in this list
		 * that supports s2 (s1 will always be of this type)
		 */
		std::vector<std::pair<Operator, Function*>> operators;
	};
} // namespace ds