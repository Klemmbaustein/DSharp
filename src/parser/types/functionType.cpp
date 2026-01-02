#include <ds/parser/types/functionType.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/service/languageService.hpp>
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
	applyName();
}

ExpressionResult ds::FunctionType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::FunctionType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	auto fn = with->scopeFile->getMethod(first, line, errors);

	if (!fn)
	{
		return ExpressionResult();
	}

	#ifdef WITH_LANGUAGE_SERVICE
	if (with->context->service)
	{
		with->context->service->files[with->scopeFile->name]
			.functions.push_back(ScannedFunction(fn, first, ScannedFunction::Kind::functionReference));
	}
#endif

	ExpressionResult result;

	std::vector<FunctionArgument> fnArgs = fn->getArguments();
	std::vector<Type*> args;

	for (FunctionArgument& i : fnArgs)
	{
		args.push_back(i.type);
	}

	result.type = getInstance(fn->getReturnType(), args, with->context->registry);
	result.code = fn->compileCallable(errors, with, hintType);
	if (result.code.instructions.empty())
	{
		return ExpressionResult();
	}
	result.code.addBuffer(compileEndMove(with));
	result.valid = true;
	return result;
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
		return getInstance(nullptr, types, file->context->registry);
	}

	line.get();

	return getInstance(file->getType(line, errors), types, file->context->registry);
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

	for (size_t i = 0; i < arguments.size(); i++)
	{
		result.append(Type::toString(arguments[i]));
		if (i != arguments.size() - 1)
		{
			result.append(", ");
		}
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