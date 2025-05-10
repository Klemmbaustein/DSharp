#pragma once
#include "type.hpp"

namespace lang
{
	struct ClassMember
	{
		Token name;
		uint32_t offset = 0;
		Type* type = nullptr;
	};

	class ClassType : public Type
	{
	public:
		ClassType(bool isReference = false)
		{
			this->size = sizeof(size_t);

			if (isReference)
			{
				this->classReference = new ClassType(true);
				*this->classReference = *this;
				this->classReference->isReference = true;
			}
		}

		~ClassType() override
		{
			delete this->classReference;
		}

		size_t classSize = 0;
		Function* baseConstructor = nullptr;
		Function* destructor = nullptr;
		ClassType* classReference = nullptr;
		std::vector<ClassMember> members;
		std::map<std::string, Function*> methods;
		std::vector<Function*> constructors;
		bool isReference = false;

		BytecodeBuffer compileUnref() override;
		BytecodeBuffer compileMove(ParsedScope* with) override;
		BytecodeBuffer compileEndMove(ParsedScope* with) override;

		virtual bool sameAs(Type* other)
		{
			auto classType = dynamic_cast<ClassType*>(other);

			return this == classType || this->classReference == classType || classType->classReference == this;
		}

		virtual ExpressionResult compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second) override;
		virtual ExpressionResult compileValue(Token first, TokenLine& line, ErrorContext* errors, ParsedScope* with) override;
		virtual ExpressionResult compileCast(ExpressionResult value) override;
		virtual ExpressionResult compileMember(ExpressionResult value, TokenLine& line, ErrorContext* errors, bool setMember, ParsedScope* with) override;
	};
} // namespace lang
