#include <ds/native/nativeStructType.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/parser.hpp>
#include <ds/service/languageService.hpp>
#include <ds/parser/parseExpression.hpp>

using namespace ds;

ds::NativeStructType::NativeStructType(Size size, std::string name)
{
	this->size = size;
	this->name = name;
}
BytecodeBuffer ds::NativeStructType::compileUnref()
{
	return Type::compileUnref();
}

BytecodeBuffer ds::NativeStructType::compileMove(ParsedScope* with)
{
	return Type::compileMove(with);
}

BytecodeBuffer ds::NativeStructType::compileEndMove(ParsedScope* with)
{
	return Type::compileEndMove(with);
}

ExpressionResult ds::NativeStructType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	for (auto& [op, fn] : this->operators)
	{
		if (op != operatorType)
		{
			continue;
		}
		ExpressionResult result;
		result.valid = true;
		result.code.addBuffer(first.code);
		result.code.addBuffer(second.code);

		auto args = fn->getArguments();

		if (args.size() == 2)
		{
			if (!args[0].type->sameAs(first.type))
			{
				continue;
			}
			if (!args[1].type->sameAs(second.type))
			{
				continue;
			}
		}

		if (args.size() == 1)
		{
			if (!args[0].type->sameAs(first.type))
			{
				continue;
			}
			if (second.type)
			{
				continue;
			}
		}

		auto fnCall = fn->compileCall();

		result.code.addBuffer(fnCall.code);
		result.type = fnCall.type;
		result.valid = fnCall.valid;

		return result;
	}
	return ExpressionResult();
}

ExpressionResult ds::NativeStructType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	auto constructorArgs = line.getInBraces(errors);
	auto argsEnd = line.previous();
	TokenLine argsLine;
	argsLine.lineTokens = &constructorArgs;

	auto initialPosition = argsLine.savePosition();
	ExpressionResult result;
	for (auto& i : constructors)
	{

		result = Expression::parseFunctionArguments(first.string, i->getArguments(),
			argsLine, errors, false, with);
		if (result.valid)
		{
#ifdef WITH_LANGUAGE_SERVICE
			if (with->context->service)
			{
				with->context->service->files[with->scopeFile->name]
					.functions.push_back(ScannedFunction(i, first,
						ScannedFunction::Kind::constructor, argsEnd.position));
			}
#endif

			result.code.addBuffer(i->compileCall().code);
			break;
		}
		argsLine.loadPosition(initialPosition);
	}

	if (!constructors.empty() && !result.valid)
	{
		errors->error(ErrorCode::parseNoMatchingConstructor, first, "No matching constructor found.");
	}
	else if (constructors.empty())
	{
		if (!allowEmpty)
		{
			errors->error(ErrorCode::parseNoMatchingConstructor, first, "No matching constructor found.");
			return ExpressionResult();
		}
		argsLine.expectEndOfLine(errors);

		BinaryBuffer buffer;
		for (Size i = 0; i < this->size; i++)
		{
			buffer.addValue<uint8_t>(0);
		}
		result.code.addOperation(BytecodeOp::push, buffer);
	}
	result.type = this;
	result.valid = true;
	return result;
}

ExpressionResult ds::NativeStructType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}


ExpressionResult ds::NativeStructType::compileMember(ExpressionResult value, TokenLine& line,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	auto next = line.get();

	if (line.peek() == "(")
	{
		auto inBraces = line.getInBraces(errors);
		auto argsEnd = line.previous();


		for (auto& [name, function] : this->methods)
		{
			if (name == "delete")
			{
				continue;
			}
			if (name != next.string)
			{
				continue;
			}
			TokenLine argsLine;
			argsLine.lineTokens = &inBraces;

			auto functionArgs = function.function->getArguments();
			ExpressionResult callCode = Expression::parseFunctionArguments(next, functionArgs, argsLine,
				errors, true, with);
			callCode.code.addBuffer(value.code);
			auto compiled = function.function->compileCall();
			if (compiled.type)
			{
				compiled.code.addBuffer(compiled.type->compileEndMove(with));
			}

#ifdef WITH_LANGUAGE_SERVICE
			if (with->context->service)
			{
				with->context->service->files[with->scopeFile->name]
					.functions.push_back(ScannedFunction(function.function, next,
						ScannedFunction::Kind::functionCall, argsEnd.position));
			}
#endif

			callCode.type = compiled.type;
			callCode.code.addBuffer(compiled.code);
			callCode.valid = true;

			return callCode;
		}

		return ExpressionResult();
	}

	for (auto& i : this->members)
	{
		if (i.name.string == next.string)
		{
			ExpressionResult result;
			BinaryBuffer args;
			args.addValue<Size>(i.type->size);
			args.addValue<Size>(Size(this->size - i.offset - i.type->size));
			args.addValue<Size>(this->size);
			result.code = value.code;
			result.code.addOperation(BytecodeOp::getStructMember, args);

#ifdef WITH_LANGUAGE_SERVICE
			if (with->context->service)
			{
				with->context->service->files[with->scopeFile->name].variables
					.push_back(ScannedVariable(&i, this, nullptr, next));
			}
#endif


			result.valid = true;
			result.setCode = {};
			if (setMember && value.setCode)
			{
				result.setCode = value.code;
				result.setCode->addOperation(BytecodeOp::setStructMember, args);
				result.setCode->addBuffer(*value.setCode);
			}

			result.type = i.type;

			return result;
		}
	}

	return ExpressionResult();
}

void ds::NativeStructType::addConstructor(Function* newConstructor)
{
	this->constructors.push_back(newConstructor);
}