#include <ds/parser/types/functionType.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/types/builtinClassFunction.hpp>
using namespace ds;

ds::FunctionType::FunctionType(Type* returnType, std::vector<Type*> arguments)
{
	this->returnType = returnType;
	this->arguments = arguments;
	this->name = getFunctionTypeName(returnType, arguments);

	std::vector<FunctionArgument> args;
	for (auto& i : arguments)
	{
		args.push_back(FunctionArgument(i, "args"));
	}

	this->methods.insert({ "call",
		new BuiltinClassFunction(args, returnType, "call", 1, this) });
}

ExpressionResult ds::FunctionType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::FunctionType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	auto fn = with->scopeFile->getMethod(first.string);

	if (fn)
	{
		ExpressionResult result;

		std::vector<Type*> args;

		std::vector<FunctionArgument> fnArgs = fn->getArguments();

		for (FunctionArgument& i : fnArgs)
		{
			args.push_back(i.type);
		}

		result.type = getInstance(fn->getReturnType(), args);
		result.code = fn->compileCallable(errors, with, hintType);
		if (result.code.instructions.empty())
		{
			return ExpressionResult();
		}
		result.code.addBuffer(compileEndMove(with));
		result.valid = true;
		return result;
	}

	return ExpressionResult();
}

FunctionType* ds::FunctionType::compileType(Token first, TokenLine& line, ErrorContext* errors,
	ParsedFile* file)
{
	if (first != "fn")
	{
		return nullptr;
	}

	if (line.peek() != "(")
	{
		return nullptr;
	}

	TokenLine argsLine;
	auto args = line.getInBraces(errors);
	argsLine.lineTokens = &args;

	std::vector<Type*> types;

	while (!argsLine.empty())
	{
		auto found = file->getType(argsLine, errors);

		if (!found)
		{
			errors->error(ErrorCode::parseInvalidType, argsLine.get(), "Expected a type.");
			break;
		}

		types.push_back(found);
	}

	argsLine.expectEndOfLine(errors);

	auto next = line.peek();

	if (next != "->")
	{
		return getInstance(nullptr, types);
	}

	line.get();

	return getInstance(file->getType(line, errors), types);
}

ExpressionResult ds::FunctionType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ClassType::compileCast(value, with);
}

ExpressionResult ds::FunctionType::compileMember(ExpressionResult value, TokenLine& line,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	return ClassType::compileMember(value, line, errors, setMember, with);
}

ExpressionResult ds::FunctionType::compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	return ExpressionResult();
}

std::string ds::FunctionType::getFunctionTypeName(Type* returnType, const std::vector<Type*>& arguments)
{
	std::string result = "fn(";

	for (auto& i : arguments)
	{
		result.append(Type::toString(i));
	}

	if (returnType)
	{
		result.append(") -> ");
		result.append(Type::toString(returnType));
	}
	else
	{
		result.push_back(')');
	}
	return result;
}