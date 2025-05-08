#pragma once
#include "tokens.hpp"
#include "type.hpp"
#include "attribute.hpp"
#include "function.hpp"
#include "module.hpp"
#include "classType.hpp"

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
		Token name;
		TokenPos start;
		TokenPos end;
		TokenStream classStream;
		size_t refVariableCount = 0;

		std::map<Token, ParsedClassMember> members;
		std::vector<ParsedFunction*> methods;

		ClassLifetimeFunction constructor;
		ClassLifetimeFunction baseDestructor;
		Function* usedDestructor = nullptr;
		Type* thisType = nullptr;
		Module* classModule = nullptr;

		void registerType(ParseContext* context, ParsedFile* file);

		void compile(ParseContext* context, ErrorContext* errors, ParsedFile* file);

		void scan(ErrorContext* errors, ParsedFile* file);
		bool scanLine(ErrorContext* errors, ParsedFile* file);
	};
} // namespace lang