#include <ds/parser/types/lambdaType.hpp>
#include <ds/parser/types/functionType.hpp>
#include <ds/parser/parseScope.hpp>
using namespace ds;

ExpressionResult ds::LambdaType::compileValue(Token first, TokenLine& line, ErrorContext* errors,
	ParsedScope* with, Type* hintType)
{
	bool isAsync = false;

	if (first == "async")
	{
		isAsync = true;
		first = line.get();
	}

	if (first != "fn")
	{
		return ExpressionResult();
	}

	auto argTokens = line.getInBraces(errors);
	Type* returnType = nullptr;

	if (line.peek() == "->")
	{
		line.get();
		returnType = with->scopeFile->getType(line, errors);
	}

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
	newFunction.isAsync = isAsync;
	newFunction.name = Token(with->scopeFunction->name.string + ".<lambda" + std::to_string(with->lambdaCount++) + ">");
	newFunction.functionModule = with->scopeFunction->functionModule;
	newFunction.isLambda = true;
	newFunction.functionFile = with->scopeFile;

	newFunction.argumentTokens = argTokens;
	newFunction.resolveTypes(with->context, errors);
	newFunction.returnType = returnType;

	newFunction.registerFunction(with->context);

	with->parseSubScope(with->scopeFile, errors, nullptr, nullptr, with->depth,
		ParsedScope::ScopeOptions{
			.targetBuffer = newFunction.functionCode,
			.scopeTokens = hasStream ? & stream : nullptr,
			.scopeFunction = &newFunction,
			.isLambda = true,
		});

	ExpressionResult result;
	result.valid = true;

	std::vector<FunctionArgument> fnArgs = newFunction.getArguments();
	std::vector<Type*> args;

	for (FunctionArgument& i : fnArgs)
	{
		args.push_back(i.type);
	}

	result.type = FunctionType::getInstance(newFunction.returnType, args, with->context->registry);

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

ExpressionResult ds::LambdaType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::LambdaType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ParsedFunction* ds::LambdaType::compileDestructorFor(std::vector<ScopeVariable*> variables, Token lambdaName, ParsedScope* with)
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
	newFunction.registerFunction(with->context);
	newFunction.functionCode->name = newFunction.name.string;
	newFunction.functionCode->instructions = code.instructions;
	return &newFunction;
}
