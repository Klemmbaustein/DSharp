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
	class ClassType;

	/**
	 * @brief
	 * A language type, like int, string, or any user defined class.
	 */
	class Type
	{
	public:
		/// The token that defined this class. Can be empty if it doesn't have a clear definition.
		Token from;
		/// A hash of this type's full module path and name, to uniquely represent this type.
		TypeId id = 0;
		/// The size of this type in bytes.
		uint32_t size = 0;
		/// The name of this type.
		std::string name;
		/// The module this type is in.
		std::string module;
		/// Does this type have a default value that can be used when a variable isn't explicitly initialized.
		bool hasDefaultValue = true;
		/// Is this type generic (does it take types as arguments)
		bool isGeneric = false;
		/**
		 * @brief
		 * Should this type's generic arguments be added to it's name returned by Type::toString() in the form of <T1, T2...>.
		 *
		 * A function reference for example is a generic type (it's arguments and return type are it's generic arguments)
		 * but it shouldn't have the default generic name because the syntax is fn(Arg1, Arg2) -> Return not fn<Return, Arg1, Arg2>
		 */
		bool addGenericToName = true;

		/**
		 * @brief
		 * Generates the type id of this type using it's name and module path.
		 *
		 * @see ds::Type::id
		 */
		virtual void applyName();
		Type() = default;

		/// Default constructor
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

		/**
		 * @brief
		 * Gets this type as a class type, if possible.
		 *
		 * @return
		 * This type as a class type, or nullptr if this type isn't a class.
		 */
		virtual ClassType* asClass()
		{
			return nullptr;
		}

		/**
		 * @brief
		 * Gets this type's default value, used when a variable isn't initialized.
		 * @return
		 * The default value
		 */
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
		static std::string toFullString(Type* target);

		virtual std::string getName()
		{
			return this->name;
		}

	protected:
		/// A type should only be copyable by itself (for generics)
		Type(const Type&) = default;
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

		ExpressionResult compileEqualsTo(ExpressionResult first, ExpressionResult second, Token opToken,
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
		constexpr static TypeId BOOL_ID = typeIdFromName("bool");
	};
} // namespace ds