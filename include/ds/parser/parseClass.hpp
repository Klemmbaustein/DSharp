#pragma once
#include "tokens.hpp"
#include "types/type.hpp"
#include "attribute.hpp"
#include "function.hpp"
#include "module.hpp"
#include "types/classType.hpp"

namespace ds
{
	struct ParsedFile;
	struct ParseContext;
	struct ParsedFunction;
	struct ParsedClass;

	struct ParsedClassMember : public Attributable
	{
		Type* type;
		Token name;
		std::vector<Token> value;
		uint32_t offset = 0;
		bool isDerived = false;

		BytecodeBuffer readValue() const;
		BytecodeBuffer writeValue() const;
	};

	// Either a constructor or destructor
	struct ClassLifetimeFunction : public Function
	{
		BytecodeBuffer code;
		ParsedClass* parent = nullptr;
		bool isConstructor = true;

		ExpressionResult compileCall() override;

		std::vector<FunctionArgument> getArguments() override;
		virtual Type* getReturnType() override
		{
			return nullptr;
		}

		std::string getShortName() const override;
		std::string getFullName() const override;
		bool discardable() const override;
	};

	struct ParsedClass : public Attributable
	{
		ParsedClass() = default;
		~ParsedClass();

		Token name;
		std::vector<std::vector<Token>> derivedFrom;
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

		Function* getDefaultConstructor();

		bool scanned = false;

		void registerType(ParseContext* context, ParsedFile* file);
		void scanClass(ParseContext* context, ParsedFile* file);

		void compile(ParseContext* context, ErrorContext* errors, ParsedFile* file);

		void scan(ErrorContext* errors, ParsedFile* file);
		bool scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors, ParsedFile* file);

		void compileDestructor(ParseContext* context, ErrorContext* errors, ParsedFile* file);
	};
} // namespace ds