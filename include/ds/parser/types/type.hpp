#pragma once
#include <ds/parser/operator.hpp>
#include <ds/parser/expression.hpp>
#include <ds/parser/error.hpp>
#include <ds/languageTypes.hpp>
#include <ds/service/scannedSymbols.hpp>
#include <ds/parser/generic.hpp>
#include <ds/parser/typeRegistry.hpp>

namespace ds
{
	class Function;
	struct ParsedScope;
	struct ParsedFile;

	class Type
	{
	public:
		Token from;
		TypeId id = 0;
		uint32_t size = 0;
		std::string name;
		bool hasDefaultValue = true;
		bool isGeneric = false;

		virtual std::string getName()
		{
			return this->name;
		}

		virtual void applyName();
		Type() = default;
		Type(const Type&) = default;

		virtual ~Type() = default;

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) = 0;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) = 0;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) = 0;

		virtual std::vector<GenericArgument> getGenericArguments()
		{
			return {};
		}

		virtual std::vector<Type*> getGenericTypes()
		{
			return {};
		}

		virtual Type* instantiateGeneric(std::vector<Type*> types, Token at, ErrorContext* with,
			TypeRegistry* registry)
		{
			return nullptr;
		}

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

#ifdef WITH_LANGUAGE_SERVICE
		virtual ScannedType toScanned();
#endif

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
			this->size = sizeof(Int);
			applyName();
		}

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileToString(ExpressionResult thisValue,
			ErrorContext* errors, ParsedScope* with);

		constexpr static TypeId INT_ID = typeIdFromName("int");
	};

	class CharType : public PrimitiveType
	{
	public:
		CharType()
		{
			this->name = "char";
			this->size = sizeof(Char);
			applyName();
		}

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
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
			this->size = sizeof(Float);
			applyName();
		}

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileToString(ExpressionResult thisValue,
			ErrorContext* errors, ParsedScope* with);
		constexpr static TypeId FLOAT_ID = typeIdFromName("float");
	};

	class BoolType : public PrimitiveType
	{
	public:
		BoolType()
		{
			this->name = "bool";
			this->size = sizeof(Bool);
			applyName();
		}

		virtual ExpressionResult compileOperator(Operator operatorType,
			ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;

		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
	};
} // namespace ds