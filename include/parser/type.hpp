#pragma once
#include "tokens.hpp"
#include "operator.hpp"
#include "expression.hpp"
#include "error.hpp"

namespace lang
{
	class Function;
	struct ParsedScope;

	class Type
	{
	public:
		Token from;
		uint32_t size = 0;
		std::string name;

		virtual ~Type() = default;

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second) = 0;

		virtual bool sameAs(Type* other)
		{
			return this == other;
		}
		virtual BytecodeBuffer compileUnref()
		{
			return BytecodeBuffer();
		}
		virtual BytecodeBuffer compileMove(ParsedScope* with)
		{
			return BytecodeBuffer();
		}
		virtual BytecodeBuffer compileEndMove(ParsedScope* with)
		{
			return BytecodeBuffer();
		}

		virtual ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors, ParsedScope* with) = 0;
		virtual ExpressionResult compileCast(ExpressionResult value) = 0;

		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line, ErrorContext* errors, bool setMember, ParsedScope* with);
		
		/**
		* @brief
		* Returns the name of the type, or <void> if the type is null.
		*/
		static std::string toString(Type* target);
	};

	class PrimitiveType : public Type
	{
	public:
	};

	class IntType : public PrimitiveType
	{
	public:
		static inline IntType* instance;

		IntType()
		{
			this->name = "int";
			this->size = sizeof(int32_t);
			instance = this;
		}

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors, ParsedScope* with) override;
		virtual ExpressionResult compileCast(ExpressionResult value) override;
	};

	class FloatType : public PrimitiveType
	{
	public:
		FloatType()
		{
			this->name = "float";
			this->size = sizeof(float);
		}

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors, ParsedScope* with) override;
		virtual ExpressionResult compileCast(ExpressionResult value) override;
	};

} // namespace lang