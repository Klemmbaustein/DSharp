#pragma once
#include "tokens.hpp"
#include "operator.hpp"
#include "expression.hpp"
#include "error.hpp"

namespace lang
{
	class Function;

	class Type
	{
	public:
		Token from;
		uint32_t size = 0;
		std::string name;

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second) = 0;

		virtual bool sameAs(Type* other)
		{
			return this == other;
		}

		virtual ExpressionResult compileValue(Token first, TokenLine& line) = 0;
		virtual ExpressionResult compileCast(ExpressionResult value) = 0;

		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line, ErrorContext* errors, bool setMember);
		
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

		virtual ExpressionResult compileValue(Token first, TokenLine& line) override;
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

		virtual ExpressionResult compileValue(Token first, TokenLine& line) override;
		virtual ExpressionResult compileCast(ExpressionResult value) override;
	};

	struct ClassMember
	{
		Token name;
		uint32_t offset = 0;
		Type* type = nullptr;
	};

	class ClassType : public Type
	{
	public:
		ClassType(Token name, std::vector<ClassMember> members, size_t classSize, Function* constructor)
		{
			this->name = name.string;
			this->members = members;
			this->size = sizeof(size_t);
			this->classSize = classSize;
			this->constructor = constructor;
		}

		size_t classSize = 0;
		Function* constructor = nullptr;
		std::vector<ClassMember> members;

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line) override;
		virtual ExpressionResult compileCast(ExpressionResult value) override;
		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line, ErrorContext* errors, bool setMember) override;
	};

} // namespace lang