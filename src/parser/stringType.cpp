#include <parser/stringType.hpp>
#include <parser/parseScope.hpp>
using namespace lang;

ExpressionResult lang::StringType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	if (second.type != this)
	{
		return ExpressionResult();
	}

	if (operatorType != Operator::add)
	{
		return ExpressionResult();
	}

	ExpressionResult result;
	result.code.addBuffer(first.code);
	result.code.addBuffer(second.code);
	result.code.addOperation(BytecodeOp::concatString);
	result.code.addBuffer(this->compileEndMove(with));
	result.type = this;
	result.valid = true;
	return result;
}

ExpressionResult lang::StringType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with)
{
	if (first.string.size() > 3 && first.string[0] == '$')
	{
		std::string content = first.string.substr(2, first.string.size() - 3);
		ExpressionResult result = compileStringValue(content, with);

		result.code.add(new BytecodeCallNative("system::format"));

		return result;
	}

	if (first.string.size() < 2 || first.string[0] != '"' || first.string[first.string.size() - 1] != '"')
	{
		return ExpressionResult();
	}

	std::string content = first.string.substr(1, first.string.size() - 2);
	return compileStringValue(content, with);
}

ExpressionResult lang::StringType::compileCast(ExpressionResult value)
{
	return ExpressionResult();
}

ExpressionResult lang::StringType::compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	if (!indexValue.type->sameAs(IntType::getInstance()))
	{
		return ExpressionResult();
	}

	ExpressionResult result;
	result.type = CharType::getInstance();
	result.valid = true;
	result.code.addBuffer(thisValue.code);
	result.code.addBuffer(indexValue.code);
	result.code.addOperation(BytecodeOp::indexString);

	if (setMember)
	{
		result.setCode = BytecodeBuffer();
		result.setCode->addBuffer(thisValue.code);

		BinaryBuffer args;
		args.addValue<uint32_t>(this->size);
		result.setCode->addBuffer(indexValue.code);
		result.setCode->addOperation(BytecodeOp::setStringIndexCopy);
		result.setCode->addBuffer(*thisValue.setCode);
	}

	return result;
}

ExpressionResult lang::StringType::compileStringValue(std::string str, ParsedScope* with)
{
	ExpressionResult result;

	// Include the null terminator as well
	size_t strLength = str.size();
	size_t dataSize = strLength + 1;
	size_t sizeOffset = sizeof(uint32_t);
	size_t fullSize = dataSize + sizeOffset;

	BinaryBuffer args;
	args.add((uint8_t*)str.data(), dataSize);
	result.code.addOperation(BytecodeOp::push, args);

	result.code.pushInt(strLength);

	args.clear();
	args.addValue<uint32_t>(fullSize);

	result.code.pushInt(fullSize);

	result.code.add(new BytecodeAllocClass(this));

	// First 4 bytes -> string size
	result.code.pushInt(0);
	result.code.pushInt(sizeOffset);
	result.code.addOperation(BytecodeOp::setClassMemberPushAgain);

	// Everything else -> string
	result.code.pushInt(sizeOffset);
	result.code.pushInt(dataSize);
	result.code.addOperation(BytecodeOp::setClassMemberPushAgain);

	result.valid = true;
	result.type = this;

	args.clear();

	args.addValue<uint32_t>(this->size);

	result.code.addOperation(BytecodeOp::copy, args);
	result.code.addBuffer(with->addTemporaryVariable(this));
	return result;
}

ExpressionResult lang::StringType::compileMember(ExpressionResult value, TokenLine& line,
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

	errors->error(ErrorCode::parseUnknowmMember, memberName, "The type " + this->name + " does not contain a member called '" + memberName.string + "'");

	return ExpressionResult();
}
