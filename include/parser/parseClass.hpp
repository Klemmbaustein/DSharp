#pragma once
#include "tokens.hpp"
#include "type.hpp"
#include "attribute.hpp"
#include "function.hpp"
#include "module.hpp"

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
	};

	struct ParsedClass;

	struct ConstructorFunction : public Function
	{
		BytecodeBuffer code;
		ParsedClass* parent = nullptr;

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

		std::vector<ParsedClassMember> members;
		std::vector<ParsedFunction*> methods;

		ConstructorFunction constructor;
		Type* thisType = nullptr;
		Module* classModule = nullptr;

		void registerType(ParseContext* context, ParsedFile* file);

		void compile(ParseContext* context, ErrorContext* errors, ParsedFile* file);

		void scan(ErrorContext* errors, ParsedFile* file);
		bool scanLine(ErrorContext* errors, ParsedFile* file);
	};
} // namespace lang