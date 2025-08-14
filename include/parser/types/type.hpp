#pragma once
#include <parser/operator.hpp>
#include <parser/expression.hpp>
#include <parser/error.hpp>

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

		virtual std::string getName()
		{
			return this->name;
		}

		virtual ~Type() = default;

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) = 0;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with) = 0;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) = 0;

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

		virtual ExpressionResult defaultValue();

		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with);

		virtual ExpressionResult compileEqualsTo(ExpressionResult first, ExpressionResult second, Token opToken,
			ErrorContext* errors, ParsedScope* with);
		virtual ExpressionResult compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
			ErrorContext* errors, bool setMember, ParsedScope* with);
		virtual ExpressionResult compileToString(ExpressionResult thisValue,
			ErrorContext* errors, ParsedScope* with);

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
		IntType()
		{
			this->name = "int";
			this->size = sizeof(int32_t);
		}

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileToString(ExpressionResult thisValue,
			ErrorContext* errors, ParsedScope* with);

		static IntType* getInstance()
		{
			if (!instance)
			{
				instance = new IntType();
			}
			return instance;
		}

	private:
		static inline IntType* instance = nullptr;
	};

	class CharType : public PrimitiveType
	{
	public:
		CharType()
		{
			this->name = "char";
			this->size = sizeof(uint8_t);
		}

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;

		static CharType* getInstance()
		{
			if (!instance)
			{
				instance = new CharType();
			}
			return instance;
		}

	private:
		static inline CharType* instance = nullptr;
	};

	class FloatType : public PrimitiveType
	{
	public:
		FloatType()
		{
			this->name = "float";
			this->size = sizeof(float);
		}

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileToString(ExpressionResult thisValue,
			ErrorContext* errors, ParsedScope* with);

		static FloatType* getInstance()
		{
			if (!instance)
			{
				instance = new FloatType();
			}
			return instance;
		}

	private:
		static inline FloatType* instance = nullptr;
	};

	class BoolType : public PrimitiveType
	{
	public:
		BoolType()
		{
			this->name = "bool";
			this->size = sizeof(bool);
		}

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;

		static BoolType* getInstance()
		{
			if (!instance)
			{
				instance = new BoolType();
			}
			return instance;
		}

	private:
		static inline BoolType* instance = nullptr;
	};
} // namespace lang