#include <ds/parser/types/type.hpp>
#include <ds/parser/stringUtils.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/types/stringType.hpp>
using namespace ds;

ExpressionResult ds::IntType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	ExpressionResult result;
	FloatType* floatValue = nullptr;
	if (with->context->registry->ifTypeIs<FloatType>(second.type, floatValue))
	{
		ExpressionResult firstCopy = first;
		firstCopy.type = second.type;
		firstCopy.code.addOperation(BytecodeOp::intToFloat);
		result.type = firstCopy.type;
		return firstCopy.type->compileOperator(operatorType, firstCopy, second, with);
	}
	else if (!with->context->registry->ifTypeIs<IntType>(second.type))
	{
		return ExpressionResult();
	}

	if (operatorType != Operator::less)
	{
		result.code.addBuffer(first.code);
		result.code.addBuffer(second.code);
	}
	else
	{
		result.code.addBuffer(second.code);
		result.code.addBuffer(first.code);
	}
	result.valid = true;

	switch (operatorType)
	{
	case ds::Operator::add:
		result.code.addOperation(BytecodeOp::addInt);
		break;
	case ds::Operator::subtract:
		result.code.addOperation(BytecodeOp::subInt);
		break;
	case ds::Operator::multiply:
		result.code.addOperation(BytecodeOp::mulInt);
		break;
	case ds::Operator::divide:
		result.code.addOperation(BytecodeOp::divInt);
		break;
	case ds::Operator::less:
	case ds::Operator::greater:
		result.code.addOperation(BytecodeOp::greaterInt);
		result.type = with->context->registry->getEntry<BoolType>();
		break;
	case ds::Operator::unaryMinus:
		result.code.addOperation(BytecodeOp::negativeInt);
		break;
	case ds::Operator::modulo:
	case ds::Operator::unknown:
	default:
		return ExpressionResult();
	}

	return result;
}

ExpressionResult ds::IntType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	bool isNegative = false;
	if (first == "+")
	{
		first = line.get();
	}
	else if (first == "-")
	{
		first = line.get();
		isNegative = true;
	}

	if (!isNumber(first.string.c_str(), false))
	{
		return ExpressionResult();
	}

	try
	{
		int32_t number = std::stoi(first.string);
		if (isNegative)
		{
			number = -number;
		}

		ExpressionResult result;
		BinaryBuffer valueBuffer;
		valueBuffer.addValue(number);
		result.code.addOperation(BytecodeOp::push, valueBuffer);
		result.valid = true;
		result.type = this;
		return result;
	}
	catch (std::exception& e)
	{
		errors->error(ErrorCode::parseInvalidType, first, e.what());
	}
	return ExpressionResult();
}

ExpressionResult ds::IntType::compileCast(ExpressionResult value, ParsedScope* with)
{
	auto floatValue = dynamic_cast<FloatType*>(value.type);
	if (floatValue)
	{
		value.code.addOperation(BytecodeOp::floatToInt);
		return value;
	}

	return ExpressionResult();
}

ExpressionResult ds::IntType::compileToString(ExpressionResult thisValue, ErrorContext* errors,
	ParsedScope* with)
{
	ExpressionResult result = thisValue;
	result.code.addNew<BytecodeCallNative>("system::int.toString");
	result.valid = true;
	result.type = with->context->registry->getEntry<StringType>();
	return result;
}

void ds::Type::applyName()
{
	this->id = typeIdFromName(getName());
}

ExpressionResult ds::Type::defaultValue()
{
	ExpressionResult result;
	result.valid = true;
	result.type = this;

	BinaryBuffer args;
	for (uint32_t i = 0; i < this->size; i++)
	{
		args.addValue<char>(0);
	}

	result.code.addOperation(BytecodeOp::push, args);
	return result;
}

ExpressionResult ds::Type::compileMember(ExpressionResult value, TokenLine& line,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::Type::compileEqualsTo(ExpressionResult first, ExpressionResult second, Token opToken,
	ErrorContext* errors, ParsedScope* with)
{
	second.compileToType(opToken, first.type, with, errors);
	if (!first.type->sameAs(second.type))
	{
		return ExpressionResult();
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

ExpressionResult ds::Type::compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
	ErrorContext* errors, bool setMember, ParsedScope* scope)
{
	return ExpressionResult();
}

ExpressionResult ds::Type::compileToString(ExpressionResult thisValue, ErrorContext* errors, ParsedScope* with)
{
	return ExpressionResult();
}

ScannedType ds::Type::toScanned()
{
	auto name = this->getName();

	return ScannedType{
		.name = name,
		.id = typeIdFromName(name),
	};
}

std::string ds::Type::toString(Type* target)
{
	if (target && target->isGeneric && target->addGenericToName)
	{
		auto args = target->getGenericTypes();
		std::string outName = target->getName();

		if (args.size())
		{
			outName.push_back('<');
			for (size_t i = 0; i < args.size(); i++)
			{
				outName.append(Type::toString(args[i]));

				if (i != args.size() - 1)
				{
					outName.append(", ");
				}
			}
			outName.push_back('>');
		}

		return outName;
	}

	return target ? target->getName() : "<void>";
}

ExpressionResult ds::FloatType::compileOperator(Operator operatorType, ExpressionResult& first,
	ExpressionResult& second, ParsedScope* with)
{
	if (second.type && !with->context->registry->ifTypeIs<FloatType>(second.type) &&
		!with->context->registry->ifTypeIs<IntType>(second.type))
	{
		return ExpressionResult();
	}


	ExpressionResult result;

	if (operatorType != Operator::less)
	{
		result.code.addBuffer(first.code);
		result.code.addBuffer(second.code);
		auto intValue = dynamic_cast<IntType*>(second.type);
		if (intValue)
		{
			result.code.addOperation(BytecodeOp::intToFloat);
		}
	}
	else
	{
		result.code.addBuffer(second.code);
		auto intValue = dynamic_cast<IntType*>(second.type);
		if (intValue)
		{
			result.code.addOperation(BytecodeOp::intToFloat);
		}
		result.code.addBuffer(first.code);
	}
	result.type = this;
	result.valid = true;

	switch (operatorType)
	{
	case ds::Operator::add:
		result.code.addOperation(BytecodeOp::addFloat);
		break;
	case ds::Operator::subtract:
		result.code.addOperation(BytecodeOp::subFloat);
		break;
	case ds::Operator::multiply:
		result.code.addOperation(BytecodeOp::mulFloat);
		break;
	case ds::Operator::divide:
		result.code.addOperation(BytecodeOp::divFloat);
		break;
	case ds::Operator::less:
	case ds::Operator::greater:
		result.code.addOperation(BytecodeOp::greaterFloat);
		result.type = with->context->registry->getEntry<BoolType>();
		break;

	case ds::Operator::unaryMinus:
		result.code.addOperation(BytecodeOp::negativeFloat);
		break;
	case ds::Operator::modulo:
	case ds::Operator::unknown:
	default:
		return ExpressionResult();
	}

	return result;
}

ExpressionResult ds::FloatType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	bool isNegative = false;
	if (first == "+")
	{
		first = line.get();
	}
	else if (first == "-")
	{
		first = line.get();
		isNegative = true;
	}

	if (!isNumber(first.string.c_str(), true))
	{
		return ExpressionResult();
	}

	float number = std::stof(first.string);

	if (isNegative)
	{
		number = -number;
	}

	ExpressionResult result;
	BinaryBuffer valueBuffer;
	valueBuffer.addValue(number);
	result.code.addOperation(BytecodeOp::push, valueBuffer);
	result.valid = true;
	result.type = this;
	return result;
}

ExpressionResult ds::FloatType::compileCast(ExpressionResult value, ParsedScope* with)
{
	auto intValue = dynamic_cast<IntType*>(value.type);
	if (intValue)
	{
		value.code.addOperation(BytecodeOp::intToFloat);
		return value;
	}

	return ExpressionResult();
}

ExpressionResult ds::FloatType::compileToString(ExpressionResult thisValue, ErrorContext* errors, ParsedScope* with)
{
	ExpressionResult result = thisValue;
	result.code.addNew<BytecodeCallNative>("system::float.toString");
	result.valid = true;
	result.type = with->context->registry->getEntry<StringType>();
	return result;
}

ExpressionResult ds::BoolType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	ExpressionResult result;

	result.code.addBuffer(first.code);

	if (second.type && !second.type->sameAs(this))
	{
		return ExpressionResult();
	}
	result.code.addBuffer(second.code);

	switch (operatorType)
	{
	case ds::Operator::logicalAnd:
		result.code.addOperation(BytecodeOp::boolAnd);
		break;
	case ds::Operator::logicalNot:
		result.code.addOperation(BytecodeOp::boolNot);
		break;
	case ds::Operator::logicalOr:
		result.code.addOperation(BytecodeOp::boolOr);
		break;
	default:
		return ExpressionResult();
	}

	result.type = this;
	result.valid = true;
	return result;
}

ExpressionResult ds::BoolType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	bool value = false;
	if (first == "true")
	{
		value = true;
	}
	else if (first != "false")
	{
		return ExpressionResult();
	}

	ExpressionResult result;
	BinaryBuffer valueBuffer;
	valueBuffer.addValue<Bool>(value);
	result.code.addOperation(BytecodeOp::push, valueBuffer);
	result.valid = true;
	result.type = this;
	return result;
}

ExpressionResult ds::BoolType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::CharType::compileOperator(Operator operatorType,
	ExpressionResult& first, ExpressionResult& second, ParsedScope* with)
{
	ExpressionResult result;

	result.code.addBuffer(first.code);
	result.code.addBuffer(second.code);

	switch (operatorType)
	{
	case ds::Operator::logicalAnd:
		result.code.addOperation(BytecodeOp::boolAnd);
		break;
	default:
		return ExpressionResult();
	}

	result.type = this;
	result.valid = true;
	return result;
}

ExpressionResult ds::CharType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	if (first.string[0] != '\'')
	{
		return ExpressionResult();
	}

	ExpressionResult result;
	BinaryBuffer valueBuffer;
	valueBuffer.addValue<char>(first.string[1]);
	result.code.addOperation(BytecodeOp::push, valueBuffer);
	result.valid = true;
	result.type = this;
	return result;
}

ExpressionResult ds::CharType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}
