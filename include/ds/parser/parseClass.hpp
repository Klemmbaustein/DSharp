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
		ParsedClass(ParsedFile* definitionFile)
		{
			this->definitionFile = definitionFile;
		}

		~ParsedClass();

		Token name;
		std::vector<std::vector<Token>> derivedFrom;
		TokenPos start;
		TokenPos end;
		TokenStream classStream;

		std::map<Token, ParsedClassMember> members;
		std::map<Token, ParsedClassMember> builtInMembers;
		std::vector<ParsedFunction*> methods;

		ClassLifetimeFunction constructor;
		ClassLifetimeFunction baseDestructor;
		Function* usedDestructor = &baseDestructor;
		ClassType* thisType = nullptr;
		Module* classModule = nullptr;
		ParsedFile* definitionFile = nullptr;

		Function* getDefaultConstructor();

		ParsedClassMember& addMember(Token name, ds::Type* type, const std::vector<ds::Token>& valueTokens,
			bool builtIn);

		bool scanned = false;
		bool isFileClass = false;
		bool isInterface = false;

		void registerType(ParseContext* context, ParsedFile* file);
		void scanClass(ParseContext* context, ParsedFile* file);

		void compile(ParseContext* context, ErrorContext* errors, ParsedFile* file);
		void scan(ErrorContext* errors, ParsedFile* file);

	private:

		bool scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors, ParsedFile* file);
		void scanDerived(BytecodeOffset& position, std::vector<ClassMember>& members,
			std::map<std::string, ClassMethod>& methods, ParseContext* context, ParsedFile* file);

		void compileDestructor(ParseContext* context, ErrorContext* errors, ParsedFile* file);
		void compileBaseConstructor(ParseContext* context, ErrorContext* errors, ParsedFile* file);
		void compileConstructor(ParseContext* context, ErrorContext* errors, ParsedFile* file);

		void handleParentClass(BytecodeOffset& vTableIndex, ClassType* parent, ParseContext* context,
			ErrorContext* errors, ParsedFile* file);
		BytecodeOffset createInterfaceVTable(ClassType* interface, ParseContext* context,
			ErrorContext* errors, ParsedFile* file);
	};
} // namespace ds