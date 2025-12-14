#pragma once
#include "parser.hpp"
#include "types/taskType.hpp"
#include "parseClass.hpp"
#include "bytecode/compileBytecodeVariables.hpp"
#include "operator.hpp"

namespace ds
{
	struct ScopeVariable
	{
		Token name;
		std::shared_ptr<BytecodePushVariable> variableInstruction = nullptr;
		ParsedScope* ownedBy = nullptr;
		size_t depth = 0;
		Type* type = nullptr;
		bool readOnly = false;
		bool isInternal = false;
		mutable uint32_t lambdaOffset = 0;

		BytecodeBuffer readValue(ParsedScope* with) const;
		BytecodeBuffer writeValue() const;
		ExpressionResult readExpression(ParsedScope* with) const;
	};

	struct VariableInfo
	{
		Token name;
		Token equals;
		ExpressionResult assignedValue;
		Type* type = nullptr;
		bool isConst = false;
		bool isVar = false;
		bool isError = false;

		void create(ParsedScope* in, ErrorContext* errors) const;
	};

	struct ParsedScope
	{
		ParsedFunction* scopeFunction = nullptr;
		ParsedFile* scopeFile = nullptr;
		BytecodeBuffer* code = nullptr;
		ParsedClass* inClass = nullptr;
		ParseContext* context = nullptr;
		TokenStream* tokenStream = nullptr;

		size_t depth = 0;

		std::map<Token, ScopeVariable> variables;

		BytecodeBuffer addTemporaryVariable(Type* type);

		ScopeVariable* thisVariable = nullptr;
		ScopeVariable* taskVariable = nullptr;
		ScopeVariable* lambdaVariable = nullptr;

		void pushVariableValue(Type* type, bool copy);
		ScopeVariable& addVariable(Token name, Type* type, ErrorContext* errors);
		BytecodeBuffer compileScopeExit(size_t toDepth, bool isEnd, bool dereferenceAll = true);

		uint32_t variableStackPosition = 0;
		uint32_t lambdaOffset = 0;
		uint32_t lambdaCount = 0;
		bool compileReturn = false;
		bool returnThis = false;
		bool isLambda = false;

		std::shared_ptr<BytecodeJumpLabel> breakTarget = nullptr;
		std::shared_ptr<BytecodeJumpLabel> continueTarget = nullptr;
		size_t breakContinueDepth = 0;
		size_t functionDepth = 0;

		void setClass(ParsedClass* inClass, bool copy);
		void addTask(TaskType* taskType);

		void compile(ParseContext* context, ParsedFile* file, ErrorContext* errors);
		void compileLine(TokenLine line, ParsedFile* file, ErrorContext* errors);
		void compileIf(TokenLine line, ParsedFile* file, ErrorContext* errors);
		void compileFor(TokenLine line, ParsedFile* file, ErrorContext* errors);

		struct ScopeOptions
		{
			BytecodeBuffer* targetBuffer = nullptr;
			TokenStream* scopeTokens = nullptr;
			ParsedFunction* scopeFunction = nullptr;
			bool isLambda = false;
		};

		void returnCompletedTask(TaskType* taskType);

		std::optional<VariableInfo> parseVariableDefinition(TokenLine& line, ParsedFile* file, ErrorContext* errors, bool matchTypes = true);

		void parseSubScope(ParsedFile* file, ErrorContext* errors, std::shared_ptr<BytecodeJumpLabel> breakTarget,
			std::shared_ptr<BytecodeJumpLabel> continueTarget, size_t breakContinueDepth,
			ScopeOptions options = ScopeOptions{
				.targetBuffer = nullptr,
				.scopeTokens = nullptr,
				.scopeFunction = nullptr,
				.isLambda = false,
			});

	private:
#ifdef WITH_LANGUAGE_SERVICE
		void serializeScope();
#endif

		uint32_t tempCounter = 0;
	};
} // namespace ds