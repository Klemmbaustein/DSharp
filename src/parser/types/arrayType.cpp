#include <parser/types/arrayType.hpp>
#include <parser/types/listType.hpp>
#include <parser/parseScope.hpp>
using namespace lang;

ExpressionResult lang::ArrayType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult lang::ArrayType::compileValue(Token first, TokenLine& line, ErrorContext* errors,
	ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult lang::ArrayType::compileCast(ExpressionResult value, ParsedScope* with)
{
	if (value.type->sameAs(ListType::getInstance()))
	{
		return makeArrayValue({}, with);
	}
	return ExpressionResult();
}

ExpressionResult lang::ArrayType::compileMember(ExpressionResult value, TokenLine& line,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	Token memberName = line.get();

	if (memberName == "length")
	{
		ExpressionResult result;
		result.code.addBuffer(value.code);
		// string length offset (0 bytse)
		result.code.pushInt(0);
		// string length size (sizeof uint32_t bytes)
		result.code.pushInt(sizeof(uint32_t));
		result.code.addOperation(BytecodeOp::classMember);
		result.type = IntType::getInstance();
		result.valid = true;
		return result;
	}
	if (memberName == "push")
	{
		auto inBraces = line.getInBraces(errors);

		TokenLine argsLine;
		argsLine.lineTokens = &inBraces;

		std::vector<FunctionArgument> functionArgs = { FunctionArgument(baseType, Token("value")) };

		ExpressionResult result = with->parseFunctionArguments(this->name + ".push", functionArgs, argsLine, errors);
		result.code.pushInt(this->baseType->size);
		result.code.addBuffer(value.code);
		result.code.add(new BytecodeCallNative("system::array.push"));
		result.valid = true;
		result.type = nullptr;

		return result;
	}


	return ExpressionResult();
}

ExpressionResult lang::ArrayType::compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	uint32_t headerSize = sizeof(uint32_t);
	uint32_t elementSize = this->baseType->size;
	ExpressionResult result;

	if (!indexValue.type->sameAs(IntType::getInstance()))
	{
		return ExpressionResult();
	}

	result.code.addBuffer(thisValue.code);
	result.code.addBuffer(indexValue.code);

	result.code.pushInt(elementSize);
	result.code.add(new BytecodeCallNative("system::array.at"));
	result.type = baseType;
	result.valid = true;
	return result;
}

ExpressionResult lang::ArrayType::makeArrayValue(std::vector<ExpressionResult> values, ParsedScope* with)
{
	ExpressionResult result;

	uint32_t elementSize = this->baseType->size;

	for (auto& i : values)
	{
		result.code.addBuffer(i.code);
		result.code.addBuffer(i.type->compileMove(with));
	}

	result.code.pushInt(values.size());

	result.code.pushInt(elementSize);
	result.code.add(new BytecodeCallNative("system::array.new"));

	result.valid = true;
	result.type = this;

	BinaryBuffer args;
	args.addValue<uint32_t>(this->size);

	result.code.addOperation(BytecodeOp::copy, args);
	result.code.addBuffer(with->addTemporaryVariable(this));

	return result;
}
