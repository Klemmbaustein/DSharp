#include <parser/types/arrayType.hpp>
#include <parser/types/listType.hpp>
#include <parser/parseScope.hpp>
#include <native/nativeModule.hpp>
using namespace lang;

class ArrayMemberFunction : public Function
{
public:
	ArrayMemberFunction(std::vector<FunctionArgument> arguments, Type* returnType,
		std::string name)
	{
		this->arguments = arguments;
		this->returnType = returnType;
		this->name = name;
	}

	std::vector<FunctionArgument> arguments;
	Type* returnType;
	std::string name;

	ExpressionResult compileCall() override
	{
		ExpressionResult result;
		result.code.addNew<BytecodeCallNative>(name);
		result.valid = true;
		result.type = returnType;
		return result;
	}
	std::vector<FunctionArgument> getArguments() override
	{
		return this->arguments;
	}
	Type* getReturnType() override
	{
		return returnType;
	}
	std::string getFullName() const override
	{
		return name;
	}
	std::string getShortName() const override
	{
		return name;
	}

	bool discardable() const override
	{
		return false;
	}
};

lang::ArrayType::ArrayType(Type* baseType)
{
	this->name = baseType->name + "[]";
	this->size = sizeof(size_t);
	this->vTableOffset = UINT32_MAX;
	this->baseType = baseType;

	this->methods.insert({ "push",
		new ArrayMemberFunction({ FunctionArgument(baseType, Token("value")) }, nullptr, "system::array.push") });

	this->methods.insert({ "pop",
		new ArrayMemberFunction({ }, nullptr, "system::array.pop") });
}

ExpressionResult lang::ArrayType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult lang::ArrayType::compileValue(Token first, TokenLine& line, ErrorContext* errors,
	ParsedScope* with, Type* hintType)
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
	Token memberName = line.peek();

	if (memberName == "length")
	{
		line.get();
		ExpressionResult result;
		result.code.addBuffer(value.code);
		// array length offset (0 bytes)
		result.code.pushInt(0);
		// array length size (sizeof uint32_t bytes)
		result.code.pushInt(sizeof(uint32_t));
		result.code.addOperation(BytecodeOp::classMember);
		result.type = IntType::getInstance();
		result.valid = true;
		return result;
	}

	value.code.pushInt(this->baseType->size);

	return ClassType::compileMember(value, line, errors, setMember, with);
}

ExpressionResult lang::ArrayType::compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	uint32_t elementSize = this->baseType->size;
	ExpressionResult result;

	if (!indexValue.type->sameAs(IntType::getInstance()))
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

	
	bool isType = dynamic_cast<ClassType*>(this->baseType) || dynamic_cast<NullableClassType*>(this->baseType);

	BinaryBuffer b;
	b.addValue(isType);
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
