#include <ds/parser/parseFunction.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/modules/system.hpp>
#include <ds/service/languageService.hpp>
#include <ds/parser/bytecode/compileBytecodeVirtual.hpp>
#include <ds/parser/types/taskType.hpp>

using namespace ds;

void ds::ParsedFunction::compile(ParseContext* context, ParsedFile* file, ErrorContext* errors)
{
	ParsedScope functionScope;
	functionScope.tokenStream = &functionStream;
	functionScope.code = &functionCode;
	functionScope.scopeFunction = this;
	functionScope.compileReturn = true;
	functionScope.scopeFile = file;
	functionScope.returnThis = this->inClass && this->name == "new";

	if (this->isAsync)
	{
		functionScope.addTask(static_cast<TaskType*>(this->returnType));
	}

	if (this->inClass)
	{
		functionScope.setClass(this->inClass, this->name != "delete");
	}

	addArguments(functionScope, errors);

	functionScope.compile(context, file, errors);

	registerFunction(context);
}

void ds::ParsedFunction::addArguments(ParsedScope& scope, ErrorContext* errors)
{
	// Read the arguments given to the function and add them as variables in the scope.
	// They're added in reverse order because they're pushed onto the stack in this order.
	for (auto it = arguments.rbegin(); it < arguments.rend(); it++)
	{
		scope.pushVariableValue(it->type, false);
		auto& var = scope.addVariable(it->name, it->type, errors);
#ifdef WITH_LANGUAGE_SERVICE
		if (scope.scopeFile->context->service)
		{
			scope.scopeFile->context->service->files[functionFile->name]
				.variables.push_back(ScannedVariable(&var, it->name));
		}
#endif
	}
}
std::optional<SymbolDefinition> ds::ParsedFunction::getDefinition()
{
	return SymbolDefinition{
		.file = functionFile ? functionFile->name : "",
		.at = name
	};
}

void ds::ParsedFunction::registerFunction(ParseContext* context)
{
	auto& bytecodeFunction = context->compiler.functions[getFullName()];
	bytecodeFunction = functionCode;
	bytecodeFunction.isEntryPoint = getAttribute<modules::system::EntryPointAttribute>();
}

std::string ds::ParsedFunction::getFullName() const
{
	if (this->inClass)
		return this->functionModule->name + "::" + this->inClass->name.string + "." + this->name.string;
	return this->functionModule->name + "::" + this->name.string;
}

std::string ds::ParsedFunction::getShortName() const
{
	return this->name.string;
}

void ds::ParsedFunction::scanDeclaration(TokenLine currentLine, TokenStream& stream, ParsedFile* file, ErrorContext* errors)
{
	name = currentLine.get();
	functionFile = file;

	argumentTokens = currentLine.getInBraces(errors);

	Token next = currentLine.get();

	// If the brace is directly after the function arguments, it has no return
	// type.
	if (next == "{")
	{
		start = next.position;
	}
	// Else it's the return type
	else if (next == "->")
	{
		returnTypeTokens = currentLine.getUntil("{", errors);
	}
	else
	{
		errors->error(ErrorCode::parseUnexpectedToken, next,
			"Unexpected '" + next.string + "' after function definition. Expected '-> [type]' or '{'");
	}
	start = next.position;

	stream.getScope(functionStream, errors);
}

void ds::ParsedFunction::resolveTypes(ParseContext* context, ErrorContext* errors)
{
	TokenLine line;

	this->arguments.clear();

	if (!argumentTokens.empty())
	{
		line.lineTokens = &argumentTokens;

		while (true)
		{
			FunctionArgument newArgument;
			newArgument.type = functionFile->getType(line, errors);
			newArgument.name = line.get();

			if (!newArgument.type)
			{
				errors->error(ErrorCode::parseUnknownSymbol, line.lineTokens->at(0),
					"Unknown type for function argument: '" + newArgument.name.string + "': '" +
						line.lineTokens->at(0).string + "'");
			}

			this->arguments.push_back(newArgument);

			if (line.empty())
				break;
			else
				line.expect(",", errors);
		}
	}

	if (!returnTypeTokens.empty())
	{
		line = TokenLine();
		line.lineTokens = &returnTypeTokens;

		this->returnType = functionFile->getType(line, errors);
		line.expectEndOfLine(errors);

		if (!returnType)
		{
			errors->error(ErrorCode::parseUnknownSymbol, line.lineTokens->at(0),
				"Unknown return type for function '" + this->getFullName() + "': '" + line.lineTokens->at(0).string +
					"'");
		}
	}

	if (isAsync)
	{
		this->returnType = TaskType::getInstance(this->returnType, context->registry);
	}
	resolveAttributes(functionFile, errors);
}

ExpressionResult ds::ParsedFunction::compileCall()
{
	ExpressionResult result;
	if (this->functionIsVirtual)
	{
		result.code.addNew<BytecodeCallVirtual>(this);
	}
	else
	{
		result.code.addNew<BytecodeCallFunction>(getFullName());
	}
	result.type = returnType;
	result.valid = true;
	result.discardable = this->discardable();
	return result;
}

std::vector<FunctionArgument> ds::ParsedFunction::getArguments()
{
	return this->arguments;
}

Type* ds::ParsedFunction::getReturnType()
{
	return this->returnType;
}

bool ds::ParsedFunction::discardable() const
{
	return getAttribute<modules::system::DiscardAttribute>();
}

BytecodeBuffer ds::ParsedFunction::compileCallable(ErrorContext* errors, ParsedScope* with, Type* hintType) const
{
	BytecodeBuffer result;
	result.addNew<BytecodeFunctionAddress>(getFullName());
	if (isLambda)
	{
		result.addNew<BytecodeCallNative>("system::fn.new.lambda");
	}
	else
	{
		result.addNew<BytecodeCallNative>("system::fn.new.bytecode");
	}
	return result;
}
