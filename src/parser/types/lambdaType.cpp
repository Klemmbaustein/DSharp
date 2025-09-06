#include <parser/types/lambdaType.hpp>
#include <parser/types/functionType.hpp>
#include <parser/parseScope.hpp>
using namespace lang;

ExpressionResult lang::LambdaType::compileValue(Token first, TokenLine& line, ErrorContext* errors,
	ParsedScope* with, Type* hintType)
{
	if (first != "fn")
	{
		return ExpressionResult();
	}

	auto args = line.getInBraces(errors);

	if (line.expect("{", errors))
	{
		return ExpressionResult();
	}

	TokenStream stream;
	bool hasStream = !line.empty();

	if (hasStream)
	{
		auto functionBody = line.getUntil("}", errors);
		stream.fromTokens(functionBody);
	}

	auto& newFunction = with->scopeFile->functions.emplace_back();
	newFunction.name = Token(with->scopeFunction->name.string + ".<lambda" + std::to_string(with->lambdaCount++) + ">");
	newFunction.functionModule = with->scopeFunction->functionModule;
	newFunction.isLambda = true;

	with->parseSubScope(with->scopeFile, errors, nullptr, nullptr, with->depth,
		ParsedScope::ScopeOptions{
			.targetBuffer = &newFunction.functionCode,
			.scopeTokens = hasStream ? & stream : nullptr,
			.scopeFunction = &newFunction,
			.isLambda = true,
		});

	ExpressionResult result;
	result.valid = true;
	result.type = FunctionType::getInstance(newFunction.returnType, {});

	std::vector<ScopeVariable*> variables;

	Size totalSize = 0;

	for (auto& i : newFunction.capturedVariables)
	{
		auto& var = with->variables[i];
		variables.push_back(&var);
		totalSize += var.type->size;

		result.code.addBuffer(var.readValue(with));
		result.code.addBuffer(var.type->compileMove(with));
	}

	result.code.addNew<BytecodeFunctionAddress>(compileDestructorFor(variables, newFunction.name, with)->getFullName());
	result.code.pushInt(totalSize);
	result.code.addBuffer(newFunction.compileCallable(errors, with, hintType));
	result.code.addBuffer(result.type->compileEndMove(with));

	return result;
}

ExpressionResult lang::LambdaType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult lang::LambdaType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ParsedFunction* lang::LambdaType::compileDestructorFor(std::vector<ScopeVariable*> variables, Token lambdaName, ParsedScope* with)
{
	BytecodeBuffer code;
	for (auto i = variables.rbegin(); i < variables.rend(); i++)
	{
		auto unrefCode = (*i)->type->compileUnref();

		if (unrefCode.instructions.empty())
		{
			BinaryBuffer args;
			args.addValue((*i)->type->size),
			code.addOperation(BytecodeOp::pop, args);
		}
		else
		{
			code.addBuffer(unrefCode);
		}
	}

	auto& newFunction = with->scopeFile->functions.emplace_back();
	newFunction.name = Token(lambdaName.string + ".delete");
	newFunction.functionModule = with->scopeFunction->functionModule;
	newFunction.functionCode.name = newFunction.name.string;
	newFunction.functionCode.instructions = code.instructions;
	return &newFunction;
}
