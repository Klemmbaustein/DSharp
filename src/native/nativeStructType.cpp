#include <native/nativeStructType.hpp>
#include <parser/parseScope.hpp>

using namespace lang;

lang::NativeStructType::NativeStructType(Size size, std::string name)
{
	this->size = size;
	this->name = name;
}

std::string lang::NativeStructType::getName()
{
	return this->name;
}

ExpressionResult lang::NativeStructType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult lang::NativeStructType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	auto constructorArgs = line.getInBraces(errors);
	TokenLine argsLine;
	argsLine.lineTokens = &constructorArgs;
	ExpressionResult result = with->parseFunctionArguments(first.string, {}, argsLine, errors);

	BinaryBuffer buffer;
	for (Size i = 0; i < this->size; i++)
	{
		buffer.addValue<uint8_t>(0);
	}
	result.type = this;
	result.valid = true;
	result.code.addOperation(BytecodeOp::push, buffer);
	return result;
}

ExpressionResult lang::NativeStructType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult lang::NativeStructType::compileMember(ExpressionResult value, TokenLine& line,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	auto next = line.peek();

	for (auto& i : this->members)
	{
		if (i.name == next.string)
		{
			line.get();
			ExpressionResult result;
			BinaryBuffer args;
			args.addValue<Size>(i.memberType->size);
			args.addValue<Size>(Size(this->size - i.offset - i.memberType->size));
			args.addValue<Size>(this->size);
			result.code = value.code;
			result.code.addOperation(BytecodeOp::getStructMember, args);

			result.valid = true;
			result.setCode = {};
			if (setMember && value.setCode)
			{
				result.setCode = value.code;
				result.setCode->addOperation(BytecodeOp::setStructMember, args);
				result.setCode->addBuffer(*value.setCode);
			}

			result.type = i.memberType;

			return result;
		}
	}
	return ExpressionResult();
}