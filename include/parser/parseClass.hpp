#pragma once
#include "tokens.hpp"
#include "types/type.hpp"
#include "attribute.hpp"
#include "function.hpp"
#include "module.hpp"
#include "types/classType.hpp"

namespace lang
{
	struct ParsedFile;
	struct ParseContext;
	struct ParsedFunction;

	struct ParsedClassMember
	{
		Type* type;
		Token name;
		std::vector<Token> value;
		uint32_t offset = 0;

		BytecodeBuffer readValue() const;
		BytecodeBuffer writeValue() const;
	};

	struct ParsedClass;

	// Either a constructor or destructor
	struct ClassLifetimeFunction : public Function
	{
		BytecodeBuffer code;
		ParsedClass* parent = nullptr;
		bool isConstructor = true;

		ExpressionResult compileCall() override;

		std::vector<FunctionArgument> getArguments() override;

		std::string getFullName() const override;
		bool discardable() const override;
	};

	struct ParsedClass : public Attributable
	{
		ParsedClass() = default;
		~ParsedClass();

		Token name;
		TokenPos start;
		TokenPos end;
		TokenStream classStream;
		size_t refVariableCount = 0;

		std::map<Token, ParsedClassMember> members;
		std::vector<ParsedFunction*> methods;

		ClassLifetimeFunction constructor;
		ClassLifetimeFunction baseDestructor;
		Function* usedDestructor = &baseDestructor;
		ClassType* thisType = nullptr;
		Module* classModule = nullptr;

		void registerType(ParseContext* context, ParsedFile* file);

		void compile(ParseContext* context, ErrorContext* errors, ParsedFile* file);

		void scan(ErrorContext* errors, ParsedFile* file);
		bool scanLine(ErrorContext* errors, ParsedFile* file);

		void compileDestructor(ParseContext* context, ErrorContext* errors, ParsedFile* file);
	};
} // namespace lang