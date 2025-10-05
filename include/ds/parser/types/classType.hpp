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
	};

	class ClassType;

	class NullableClassType : public Type
	{

	public:
		ClassType* from = nullptr;
		NullableClassType(ClassType* from);
		BytecodeBuffer compileUnref() override;
		BytecodeBuffer compileMove(ParsedScope* with) override;
		BytecodeBuffer compileEndMove(ParsedScope* with) override;

		std::string getName() override;

		bool isSubclassOf(ClassType* parent);

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;

		ExpressionResult compileEqualsTo(ExpressionResult first, ExpressionResult second, Token opToken,
			ErrorContext* errors, ParsedScope* with) override;

		BytecodeBuffer compileNullCheck() const;
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
			delete this->nullable;
		}

		size_t classSize = 0;
		bytecodeOffset vTableOffset = 0;
		Function* baseConstructor = nullptr;
		Function* destructor = nullptr;
		ParsedClass* languageClass = nullptr;
		std::vector<ClassMember> members;
		std::map<std::string, Function*> methods;
		std::vector<Function*> constructors;
		NullableClassType* nullable = nullptr;

		BytecodeBuffer compileUnref() override;
		BytecodeBuffer compileMove(ParsedScope* with) override;
		BytecodeBuffer compileEndMove(ParsedScope* with) override;

		bool isSubclassOf(ClassType* parent);

		std::vector<ClassType*> parents;

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with, Type* hintType) override;
		virtual ExpressionResult compileCast(ExpressionResult value, ParsedScope* with) override;
		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;
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
		ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second, ParsedScope* with) override;
		ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors, ParsedScope* with, Type* hintType) override;
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
