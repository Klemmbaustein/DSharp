#pragma once
#include "type.hpp"

namespace lang
{
	struct ParsedClass;

	struct ClassMember
	{
		Token name;
		uint32_t offset = 0;
		Type* type = nullptr;
	};

	class ClassType : public Type
	{
	public:
		ClassType()
		{
			this->size = sizeof(size_t);
		}

		~ClassType() override
		{
		}

		size_t classSize = 0;
		bytecodeOffset vTableOffset = 0;
		Function* baseConstructor = nullptr;
		Function* destructor = nullptr;
		ParsedClass* languageClass = nullptr;
		std::vector<ClassMember> members;
		std::map<std::string, Function*> methods;
		std::vector<Function*> constructors;

		BytecodeBuffer compileUnref() override;
		BytecodeBuffer compileMove(ParsedScope* with) override;
		BytecodeBuffer compileEndMove(ParsedScope* with) override;

		virtual bool sameAs(Type* other)
		{
			return this == other;
		}

		bool isSubclassOf(ClassType* parent);

		std::vector<ClassType*> parents;

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first,
			ExpressionResult& second, ParsedScope* with) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line,
			ErrorContext* errors, ParsedScope* with) override;
		virtual ExpressionResult compileCast(ExpressionResult value) override;
		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line,
			ErrorContext* errors, bool setMember, ParsedScope* with) override;
	};
} // namespace lang
