#include <ds/parser/types/classType.hpp>
#include <ds/parser/function.hpp>
#include <ds/service/languageService.hpp>
#include <ds/parser/parseScope.hpp>

using namespace ds;

BytecodeBuffer ds::ClassType::compileUnref()
{
	BytecodeBuffer outBuffer;

	outBuffer.addOperation(BytecodeOp::unrefClass);

	return outBuffer;
}

BytecodeBuffer ds::ClassType::compileEndMove(ParsedScope* with)
{
	BytecodeBuffer outBuffer;
	BinaryBuffer args;
	args.addValue<uint32_t>(this->size);
	outBuffer.addOperation(BytecodeOp::copy, args);
	outBuffer.addBuffer(with->addTemporaryVariable(this));
	return outBuffer;
}

BytecodeBuffer ds::ClassType::compileMove(ParsedScope* with)
{
	BytecodeBuffer outBuffer;
	BinaryBuffer args;

	outBuffer.addOperation(BytecodeOp::refClass);

	return outBuffer;
}

ds::NullableClassType::NullableClassType(ClassType* from)
{
	this->size = from->size;
	this->from = from;
	this->name = from->name + "?";
}

BytecodeBuffer ds::NullableClassType::compileUnref()
{
	return from->compileUnref();
}

BytecodeBuffer ds::NullableClassType::compileMove(ParsedScope* with)
{
	return from->compileMove(with);
}

BytecodeBuffer ds::NullableClassType::compileEndMove(ParsedScope* with)
{
	return from->compileEndMove(with);
}

std::string ds::NullableClassType::getName()
{
	return from->getName() + "?";
}
bool ds::ClassType::isSubclassOf(ClassType* parent)
{
	for (ClassType* i : parents)
	{
		if (i == parent)
		{
			return true;
		}
		if (i->isSubclassOf(parent))
		{
			return true;
		}
	}
	return false;
}
ExpressionResult ds::NullableClassType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	if (operatorType == Operator::dereference)
	{
		first.type = this->from;
		first.code.addBuffer(compileNullCheck());
		return first;
	}

	return from->compileOperator(operatorType, first, second, with);
}

ExpressionResult ds::NullableClassType::compileValue(Token first, TokenLine& line, ErrorContext* errors,
	ParsedScope* with, Type* hintType)
{
	return ExpressionResult();
}

ExpressionResult ds::NullableClassType::compileCast(ExpressionResult value, ParsedScope* with)
{
	if (!value.type)
	{
		return ExpressionResult();
	}

	if (value.type->sameAs(this->from))
	{
		value.type = this;
		return value;
	}

	auto nullValue = dynamic_cast<NullType*>(value.type);

	if (nullValue)
	{
		value.type = this;
		return value;
	}

	return from->compileCast(value, with);
}

ExpressionResult ds::NullableClassType::compileMember(ExpressionResult value, TokenLine& line, ErrorContext* errors,
	bool setMember, ParsedScope* with)
{
	value.code.addBuffer(compileNullCheck());
	return from->compileMember(value, line, errors, setMember, with);
}

ExpressionResult ds::NullableClassType::compileEqualsTo(ExpressionResult first, ExpressionResult second,
	Token opToken, ErrorContext* errors, ParsedScope* with)
{
	if (second.type->sameAs(NullType::getInstance()))
	{
		second.type = this;
	}
	else if (!second.type->sameAs(this))
	{
		return ExpressionResult();
	}

	return this->from->compileEqualsTo(first, second, opToken, errors, with);
}

BytecodeBuffer ds::NullableClassType::compileNullCheck() const
{
	BytecodeBuffer result;
	result.addOperation(BytecodeOp::nullCheck);
	return result;
}

ExpressionResult ds::ClassType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::ClassType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	ExpressionResult result;

	auto constructorArgs = line.getInBraces(errors);
	TokenLine argsLine;
	argsLine.lineTokens = &constructorArgs;

	if (this->constructors.empty())
	{
		result.code.pushInt(Size(this->classSize));
		result.code.add(std::make_shared<BytecodeAllocClass>(this));
		if (baseConstructor)
		{
			result.code.addBuffer(baseConstructor->compileCall().code);
		}
	}
	else
	{
		for (auto& i : this->constructors)
		{
			result.code.addBuffer(with->parseFunctionArguments(Token(first), i->getArguments(), argsLine, errors).code);

			result.code.pushInt(Size(this->classSize));
			result.code.add(std::make_shared<BytecodeAllocClass>(this));
			result.code.addBuffer(i->compileCall().code);
			break;
		}
	}
	result.type = this;
	result.valid = true;

	return result;
}

ExpressionResult ds::ClassType::compileCast(ExpressionResult value, ParsedScope* with)
{
	auto castValue = dynamic_cast<ClassType*>(value.type);

	if (castValue && (castValue->isSubclassOf(this)))
	{
		value.valid = true;
		return value;
	}
	return ExpressionResult();
}

ExpressionResult ds::ClassType::compileMember(ExpressionResult value, TokenLine& line,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	Token memberName = line.get();

	if (line.peek() == "(")
	{
		auto inBraces = line.getInBraces(errors);

		for (auto& [name, function] : this->methods)
		{
			if (name == "delete")
			{
				continue;
			}
			if (name != memberName.string)
			{
				continue;
			}
			TokenLine argsLine;
			argsLine.lineTokens = &inBraces;

			auto functionArgs = function->getArguments();

			ExpressionResult callCode = with->parseFunctionArguments(memberName, functionArgs, argsLine, errors);
			callCode.code.addBuffer(value.code);
			auto compiled = function->compileCall();
			if (compiled.type)
			{
				compiled.code.addBuffer(compiled.type->compileEndMove(with));
			}

#ifdef WITH_LANGUAGE_SERVICE
			if (with->context->service)
			{
				with->context->service->files[with->scopeFile->name]
					.functions.push_back(ScannedFunction(function, memberName));
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
		if (i.name.string != memberName.string)
		{
			continue;
		}

#ifdef WITH_LANGUAGE_SERVICE
		if (with->context->service)
		{
			with->context->service->files[with->scopeFile->name].variables.push_back(memberName);
		}
#endif

		ExpressionResult result;
		result.valid = true;
		result.type = i.type;
		BinaryBuffer args;
		result.code.addBuffer(value.code);
		result.code.pushInt(i.offset);
		result.code.pushInt(i.type->size);

		if (setMember)
		{
			auto unrefCode = i.type->compileUnref();
			result.setCode = BytecodeBuffer();
			if (unrefCode.instructions.size())
			{
				result.setCode->addBuffer(result.code);
				result.setCode->addOperation(BytecodeOp::classMember, args);
				result.setCode->addBuffer(unrefCode);
			}

			result.setCode->addBuffer(result.code);
			result.setCode->addOperation(BytecodeOp::setClassMember, args);
		}

		result.code.addOperation(BytecodeOp::classMember, args);

		return result;
	}

	return ExpressionResult();
}

ExpressionResult ds::NullType::compileOperator(Operator operatorType, ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::NullType::compileValue(Token first, TokenLine& line, ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	if (first == "null")
	{
		ExpressionResult result;
		result.valid = true;
		result.type = this;

		BinaryBuffer value;
		value.addValue(size_t(0));

		result.code.addOperation(BytecodeOp::push, value);
		return result;
	}
	return ExpressionResult();
}

ExpressionResult ds::NullType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}
