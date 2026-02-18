#include <ds/parser/types/classType.hpp>
#include <ds/parser/function.hpp>
#include <ds/service/languageService.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/parseExpression.hpp>

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
	this->isGeneric = from->isGeneric;
}

ds::NullableClassType::~NullableClassType()
{
	if (from)
		from->nullable = nullptr;
}

void ds::NullableClassType::applyName()
{
	this->id = typeIdFromName(this->from->getName());
}

void ds::ClassType::applyName()
{
	Type::applyName();
	nullable->applyName();
}

std::vector<GenericArgument> ds::NullableClassType::getGenericArguments()
{
	return from->getGenericArguments();
}
std::vector<Type*> ds::NullableClassType::getGenericTypes()
{
	return from->getGenericTypes();
}

Type* ds::NullableClassType::instantiateGeneric(std::vector<Type*> types, Token at, ErrorContext* with,
	TypeRegistry* registry)
{
	return static_cast<ClassType*>(from->instantiateGeneric(types, at, with, registry))->nullable;
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

#ifdef WITH_LANGUAGE_SERVICE
ScannedType ds::ClassType::toScanned()
{
	ScannedType out = Type::toScanned();

	for (auto& i : this->members)
	{
		out.members.push_back(ScannedMember{
			.memberTypeId = i.type ? i.type->id : 0,
			.memberTypeName = Type::toString(i.type),
			.name = i.name.string,
		});
	}

	for (auto& i : this->methods)
	{
		out.methods.push_back(ScannedFunction(i.second.function, Token(), ScannedFunction::Kind::classMember));
	}

	return out;
}
#endif

bool ds::ClassType::isSubclassOf(ClassType* parent)
{
	if (this->parent)
	{
		if (this->parent == parent)
		{
			return true;
		}
		else if (this->parent->isSubclassOf(parent))
		{
			return true;
		}
	}
	for (auto& [_, i] : interfaces)
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

ClassType* ds::NullableClassType::asClass()
{
	return from;
}

ClassType* ds::ClassType::asClass()
{
	return this;
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

	auto classValue = value.type->asClass();

	if (classValue && (classValue->isSubclassOf(from)))
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
		return this->from->compileEqualsTo(first, second, opToken, errors, with);

	}
	auto secondClass = second.type->asClass();

	if (!secondClass || !(secondClass->isSubclassOf(this->from) || this->from->isSubclassOf(secondClass)))
	{
		second.compileToType(opToken, first.type, with, errors);
		if (!first.type->sameAs(second.type))
		{
			return ExpressionResult();
		}
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
	auto& argsEnd = line.previous();

	TokenLine argsLine;
	argsLine.lineTokens = &constructorArgs;

	if (this->constructors.empty())
	{
		argsLine.expectEndOfLine(errors);
		result.code.pushInt(Size(this->classSize));
		result.code.add(std::make_shared<BytecodeAllocClass>(this));
		if (baseConstructor)
		{
			result.code.addBuffer(baseConstructor->compileCall().code);
		}
	}
	else
	{
		bool found = false;
		for (auto& i : this->constructors)
		{
			auto constructor = Expression::parseFunctionArguments(Token(first), i->getArguments(),
				argsLine, errors, false, with);

			if (!constructor.valid)
			{
				continue;
			}

#ifdef WITH_LANGUAGE_SERVICE
			if (with->context->service)
			{
				with->context->service->files[with->scopeFile->name]
					.functions.push_back(ScannedFunction(i, first,
						ScannedFunction::Kind::constructor, argsEnd.position));
			}
#endif

			result.code.addBuffer(constructor.code);

			result.code.pushInt(Size(this->classSize));
			result.code.add(std::make_shared<BytecodeAllocClass>(this));
			result.code.addBuffer(getClassGenericCode());
			result.code.addBuffer(i->compileCall().code);
			found = true;
			break;
		}

		if (!found)
		{
			errors->error(ErrorCode::parseNoMatchingConstructor, first, "No matching constructor found.");
			return ExpressionResult();
		}
	}


	result.type = this;
	result.valid = true;

	return result;
}

ExpressionResult ds::ClassType::toInterface(ExpressionResult expr, ClassType* interface)
{
	for (auto& [offset, type] : this->interfaces)
	{
		if (type->sameAs(interface))
		{
			BinaryBuffer args;
			args.addValue<Int>(offset);
			args.addValue<Bool>(false);
			expr.code.addOperation(BytecodeOp::castInterface, args);
			expr.type = interface;
			return expr;
		}
	}
	return ExpressionResult();
}

BytecodeOffset ds::ClassType::getInterfaceOffset(ClassType* interface)
{
	for (auto& [offset, type] : this->interfaces)
	{
		if (type->sameAs(interface))
		{
			return offset;
		}
	}
	return 0;
}

ExpressionResult ds::ClassType::compileEqualsTo(ExpressionResult first, ExpressionResult second, Token opToken,
	ErrorContext* errors, ParsedScope* with)
{
	auto secondClass = second.type->asClass();

	if (!secondClass || !(secondClass->isSubclassOf(this) || this->isSubclassOf(secondClass)))
	{
		second.compileToType(opToken, first.type, with, errors);
		if (!first.type->sameAs(second.type))
		{
			return ExpressionResult();
		}
	}

	ExpressionResult result;
	result.valid = true;
	result.type = with->context->registry->getEntry<BoolType>();

	result.code.addBuffer(first.code);
	result.code.addBuffer(second.code);

	result.code.pushInt(first.type->size);
	result.code.addOperation(BytecodeOp::equals);

	return result;
}


ExpressionResult ds::ClassType::compileCast(ExpressionResult value, ParsedScope* with)
{
	auto castValue = value.type->asClass();

	if (castValue && isInterface)
	{
		return castValue->toInterface(value, this);
	}

	if (castValue && (castValue->isSubclassOf(this)))
	{
		value.valid = true;
		return value;
	}
	return ExpressionResult();
}

void ds::ClassType::makePointerClass()
{
	for (auto& i : this->members)
	{
		i.isPointerMember = true;
	}
}

ExpressionResult ds::ClassType::compileMember(ExpressionResult value, TokenLine& line,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	Token memberName = line.get();
	auto methodExpr = compileMethod(memberName, value, line, errors, with);

	if (methodExpr.valid)
	{
		return methodExpr;
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
			auto file = this->languageClass ? this->languageClass->definitionFile : nullptr;

			with->context->service->files[with->scopeFile->name].variables.push_back(ScannedVariable(
				&i, this, file, memberName));
		}
#endif

		ExpressionResult result;
		result.valid = true;
		result.type = i.type;
		BinaryBuffer args;
		result.code.addBuffer(value.code);
		result.code.pushInt(i.offset);
		result.code.pushInt(i.type->size);

		if (setMember && !i.isConst)
		{
			auto unrefCode = i.type->compileUnref();
			result.setCode = BytecodeBuffer();
			if (unrefCode.instructions.size())
			{
				result.setCode->addBuffer(result.code);
				result.setCode->addOperation(
					i.isPointerMember ? BytecodeOp::classMemberPtr : BytecodeOp::classMember,
					args);
				result.setCode->addBuffer(unrefCode);
			}

			result.setCode->addBuffer(result.code);
			result.setCode->addOperation(
				i.isPointerMember ? BytecodeOp::setClassMemberPtr : BytecodeOp::setClassMember,
				args);
		}

		result.code.addOperation(i.isPointerMember ? BytecodeOp::classMemberPtr : BytecodeOp::classMember, args);

		return result;
	}

	return ExpressionResult();
}

BytecodeBuffer ds::ClassType::getClassGenericCode()
{
	if (!isGeneric)
	{
		return BytecodeBuffer();
	}

	return compileGenericArguments(this->getGenericTypes());
}

ExpressionResult ds::ClassType::compileMethod(Token memberName, ExpressionResult value, TokenLine& line,
	ErrorContext* errors, ParsedScope* with)
{
	if (memberName == "delete")
	{
		return ExpressionResult();
	}

	for (auto& [name, function] : this->methods)
	{
		if (name != memberName.string)
		{
			continue;
		}

		GenericParseData generic = getGenericFunctionData(function.function, line, errors, with);

		if (line.peek() != "(")
		{
#ifdef WITH_LANGUAGE_SERVICE
			if (with->context->service)
			{
				with->context->service->files[with->scopeFile->name]
					.functions.push_back(ScannedFunction(function.function, &generic, memberName,
						ScannedFunction::Kind::functionCall, memberName.position));
			}
#endif

			continue;
		}

		auto inBraces = line.getInBraces(errors);
		auto& bracesEnd = line.previous();

		TokenLine argsLine;
		argsLine.lineTokens = &inBraces;

		ExpressionResult callCode = Expression::parseFunctionArguments(memberName, generic.args,
			argsLine, errors, true, with);
		callCode.code.addBuffer(value.code);
		if (function.interfaceSource)
		{
			callCode = toInterface(value, function.interfaceSource);
		}

		callCode.code.addBuffer(generic.code);
		callCode.code.addBuffer(getClassGenericCode());
		auto compiled = function.function->compileCall();
		compiled.type = generic.returnType;
		if (compiled.type)
		{
			compiled.code.addBuffer(compiled.type->compileEndMove(with));
		}

#ifdef WITH_LANGUAGE_SERVICE
		if (with->context->service)
		{
			with->context->service->files[with->scopeFile->name]
				.functions.push_back(ScannedFunction(function.function, &generic, memberName,
					ScannedFunction::Kind::functionCall, bracesEnd.position));
		}
#endif

		callCode.discardable = compiled.discardable;
		callCode.type = compiled.type;
		callCode.code.addBuffer(compiled.code);
		callCode.valid = true;

		return callCode;
	}

	return ExpressionResult();
}

ExpressionResult ds::NullType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::NullType::compileValue(Token first, TokenLine& line, ErrorContext* errors,
	ParsedScope* with, Type* hintType)
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
