#pragma once
#include "type.hpp"
#include <ds/parser/function.hpp>

namespace ds
{
	struct ParsedClass;

	struct ClassMember
	{
		Token name;
		uint32_t offset = 0;
		Type* type = nullptr;

		bool isConst = false;
		bool isPointerMember = false;
	};

	class ClassType;

	struct ClassMethod
	{
		ClassMethod(Function* fn)
		{
			function = fn;
		}
		ClassMethod(Function* fn, ClassType* interfaceClass)
		{
			function = fn;
			interfaceSource = interfaceClass;
		}

		Function* function = nullptr;
		ClassType* interfaceSource = nullptr;
	};

	class NullableClassType : public Type
	{

	public:
		ClassType* from = nullptr;
		NullableClassType(ClassType* from);
		~NullableClassType();
		BytecodeBuffer compileUnref() override;
		BytecodeBuffer compileMove(ParsedScope* with) override;
		BytecodeBuffer compileEndMove(ParsedScope* with) override;
		void applyName() override;

		std::string getName() override;

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;
		ClassType* asClass() override;

		ExpressionResult compileEqualsTo(ExpressionResult first, ExpressionResult second, Token opToken,
			ErrorContext* errors, ParsedScope* with) override;

		BytecodeBuffer compileNullCheck() const;

		virtual std::vector<GenericArgument> getGenericArguments() override;

		std::vector<Type*> getGenericTypes() override;

		Type* instantiateGeneric(std::vector<Type*> types, Token at, ErrorContext* with,
			TypeRegistry* registry) override;
	};

	class ClassType : public Type
	{
	public:
		ClassType()
		{
			this->size = sizeof(Pointer);
			this->hasDefaultValue = false;

			nullable = new NullableClassType(this);
		}

		~ClassType() override
		{
			this->nullable->from = nullptr;
			if (this->nullable)
				delete this->nullable;
		}

		BytecodeOffset vTableOffset = 0;
		bool isInterface = false;
		size_t classSize = 0;

		void makePointerClass();
		ClassType* asClass() override;
		Function* baseConstructor = nullptr;
		Function* destructor = nullptr;
		ParsedClass* languageClass = nullptr;
		std::vector<ClassMember> members;
		std::map<std::string, ClassMethod> methods;
		std::vector<Function*> constructors;
		NullableClassType* nullable = nullptr;

#ifdef WITH_LANGUAGE_SERVICE
		virtual ScannedType toScanned();
#endif

		BytecodeBuffer compileUnref() override;
		BytecodeBuffer compileMove(ParsedScope* with) override;
		BytecodeBuffer compileEndMove(ParsedScope* with) override;

		bool isSubclassOf(ClassType* parent);
		void applyName() override;

		ClassType* parent = nullptr;
		std::map<BytecodeOffset, ClassType*> interfaces;

		ExpressionResult toInterface(ExpressionResult expr, ClassType* interface);
		BytecodeOffset getInterfaceOffset(ClassType* interface);

		ExpressionResult compileEqualsTo(ExpressionResult first, ExpressionResult second, Token opToken,
			ErrorContext* errors, ParsedScope* with) override;

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;

	private:

		BytecodeBuffer getClassGenericCode();
		ExpressionResult compileMethod(Token memberName, ExpressionResult value, TokenLine& line,
			ErrorContext* errors, ParsedScope* with);
	};

	class NullType : public Type
	{
	public:
		NullType()
		{
			this->name = "<null>";
			this->size = sizeof(Pointer);
		}

		// Inherited via Type
		ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors,
			ParsedScope* with, Type* hintType) override;
		ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;

		static NullType* getInstance()
		{
			if (!instance)
			{
				instance = new NullType();
			}
			return instance;
		}

	private:
		static inline NullType* instance = nullptr;
	};
} // namespace ds
