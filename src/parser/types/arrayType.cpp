#include <ds/parser/types/arrayType.hpp>
#include <ds/parser/types/listType.hpp>
#include <ds/parser/types/builtinClassFunction.hpp>
#include <ds/parser/parseScope.hpp>
using namespace ds;

ds::ArrayType::ArrayType(Type* baseType, TypeRegistry* registry)
{
	this->name = baseType->getName() + "[]";
	this->size = sizeof(Pointer);
	this->vTableOffset = UINT32_MAX;
	this->baseType = baseType;

	this->methods.insert({ "push",
		new BuiltinClassFunction({ FunctionArgument(baseType, Token("value")) }, nullptr, "system::array.push", "push") });

	this->methods.insert({ "pop",
		new BuiltinClassFunction({}, nullptr, "system::array.pop", "pop") });
	auto intType = registry->getEntry<IntType>();

	this->methods.insert({ "removeIndex",
		new BuiltinClassFunction({ FunctionArgument(intType, Token("value")) },
			nullptr, "system::array.removeIndex", "removeIndex") });

	this->members.push_back(ClassMember{
		.name = "length",
		.offset = 0,
		.type = intType });

	isGeneric = true;
	addGenericToName = false;

	applyName();
}

ExpressionResult ds::ArrayType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::ArrayType::compileValue(Token first, TokenLine& line, ErrorContext* errors,
	ParsedScope* with, Type* hintType)
{
	auto constructorArgs = line.getInBraces(errors);
	if (!constructorArgs.empty())
	{
		errors->error(ErrorCode::parseUnexpectedToken, first, "Unexpected arguments for constructor");
		return ExpressionResult();
	}
	auto cls = makeArrayValue({}, with);

	cls.code.addBuffer(compileMove(with));

	return cls;
}

ExpressionResult ds::ArrayType::compileCast(ExpressionResult value, ParsedScope* with)
{
	if (with->context->registry->ifTypeIs<ListType>(value.type))
	{
		return makeArrayValue({}, with);
	}
	return ExpressionResult();
}

ExpressionResult ds::ArrayType::compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	uint32_t elementSize = this->baseType->size;
	ExpressionResult result;

	if (!with->context->registry->ifTypeIs<IntType>(indexValue.type))
	{
		return ExpressionResult();
	}

	result.code.addBuffer(thisValue.code);
	result.code.addBuffer(indexValue.code);

	result.code.pushInt(elementSize);
	result.code.addNew<BytecodeCallNative>("system::array.at");
	result.type = baseType;
	result.valid = true;
	return result;
}

ExpressionResult ds::ArrayType::getLength(BytecodeBuffer thisValue, ParseContext* context)
{
	ExpressionResult result;
	result.code.addBuffer(thisValue);
	// array length offset (0 bytes)
	result.code.pushInt(0);
	// array length size (sizeof uint32_t bytes)
	result.code.pushInt(sizeof(uint32_t));
	result.code.addOperation(BytecodeOp::classMember);
	result.type = context->registry->getEntry<IntType>();
	result.valid = true;
	return result;
}

ExpressionResult ds::ArrayType::makeArrayValue(std::vector<ExpressionResult> values, ParsedScope* with)
{
	ExpressionResult result;

	uint32_t elementSize = this->baseType->size;

	for (auto& i : values)
	{
		result.code.addBuffer(i.code);
		result.code.addBuffer(i.type->compileMove(with));
	}

	result.code.pushInt(Size(values.size()));
	result.code.pushInt(elementSize);

	auto baseClass = baseType->asClass();

	Bool isClassType = baseClass && !baseClass->isByValueType;

	BinaryBuffer b;
	b.addValue(isClassType);
	result.code.addOperation(BytecodeOp::push, b);

	result.code.addNew<BytecodeCallNative>("system::array.new");

	result.valid = true;
	result.type = this;

	BinaryBuffer args;
	args.addValue<uint32_t>(this->size);

	result.code.addOperation(BytecodeOp::copy, args);
	result.code.addBuffer(with->addTemporaryVariable(this));

	return result;
}
