#include <ds/parser/types/stringType.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/varArgs.hpp>
#include <ds/parser/types/builtinClassFunction.hpp>
#include <ds/parser/parseExpression.hpp>
using namespace ds;

ds::StringType::StringType(TypeRegistry* registry)
{
	this->name = "string";
	this->size = sizeof(Pointer);
	this->vTableOffset = UINT32_MAX;
	applyName();

	auto intType = registry->getEntry<IntType>();

	this->members.push_back(ClassMember{
		.name = "length",
		.offset = 0,
		.type = registry->getEntry<IntType>(),
	});

	this->methods.insert({ "substr",
		new BuiltinClassFunction({ FunctionArgument(intType, Token("start")), FunctionArgument(intType, Token("count")) },
			this, "system::string.substr", "substr") });
}

ds::StringType::~StringType()
{
	for (auto& i : this->methods)
	{
		delete i.second.function;
	}
}

ExpressionResult ds::StringType::compileOperator(Operator operatorType, ExpressionResult& first,
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
	result.code.addNew<BytecodeCallNative>("system::string.concat");
	result.code.addBuffer(this->compileEndMove(with));
	result.type = this;
	result.valid = true;
	return result;
}

ExpressionResult ds::StringType::compileValue(Token first, TokenLine& line,
	ErrorContext* errors, ParsedScope* with, Type* hintType)
{
	if (first.string.size() > 3 && first.string[0] == '$')
	{
		return compileFormatString(first, line, errors, with);
	}

	if (first.unquoteString())
	{
		return compileStringValue(first.string, with);
	}
	return ExpressionResult();
}

ExpressionResult ds::StringType::compileCast(ExpressionResult value, ParsedScope* with)
{
	return ExpressionResult();
}

ExpressionResult ds::StringType::compileIndex(ExpressionResult thisValue, ExpressionResult indexValue,
	ErrorContext* errors, bool setMember, ParsedScope* with)
{
	if (!with->context->registry->ifTypeIs<IntType>(indexValue.type))
	{
		return ExpressionResult();
	}

	ExpressionResult result;
	result.type = with->context->registry->getEntry<CharType>();
	result.valid = true;
	result.code.addBuffer(thisValue.code);
	result.code.addBuffer(indexValue.code);
	result.code.addNew<BytecodeCallNative>("system::string.index");

	if (setMember)
	{
		result.setCode = BytecodeBuffer();
		result.setCode->addBuffer(thisValue.code);

		BinaryBuffer args;
		args.addValue<uint32_t>(this->size);
		result.setCode->addBuffer(indexValue.code);
		result.setCode->addNew<BytecodeCallNative>("system::string.setIndexCopy");
		result.setCode->addBuffer(*thisValue.setCode);
	}

	return result;
}

ExpressionResult ds::StringType::compileEqualsTo(ExpressionResult first, ExpressionResult second, Token opToken,
	ErrorContext* errors, ParsedScope* with)
{
	first.code.addOperation(BytecodeOp::refClass);
	second.compileToType(opToken, this, with, errors);
	first.code.addBuffer(second.code);
	first.code.addOperation(BytecodeOp::refClass);
	first.code.addNew<BytecodeCallNative>("system::compareString");

	// compare value, if compareString returns 0 they're the same
	first.code.pushInt(0);

	BinaryBuffer args;
	args.addValue(sizeof(uint32_t));

	first.code.addOperation(BytecodeOp::equals, args);
	first.valid = true;
	first.type = with->context->registry->getEntry<BoolType>();
	return first;
}

ExpressionResult ds::StringType::compileFormatString(Token first, TokenLine& line, ErrorContext* errors,
	ParsedScope* with)
{
	std::string content = first.string.substr(2, first.string.size() - 3);

	std::string resultString;
	std::string currentExprCode;
	bool inExpr = false;

	std::vector<ExpressionResult> formatArguments;

	uint32_t it = 2;
	uint32_t formatArgStart = 0;

	for (char c : content)
	{
		if (c == '{')
		{
			if (inExpr)
			{
				errors->error(ErrorCode::parseInvalidFormat, first,
					"Unexpected '{' after another '{' without a closing '}'");
			}
			resultString.push_back(c);
			inExpr = true;
			it++;
			formatArgStart = it;
			continue;
		}
		if (c == '}')
		{
			if (!inExpr)
			{
				errors->error(ErrorCode::parseInvalidFormat, first,
					"Unexpected '}'");
			}
			inExpr = false;
			resultString.push_back(c);

			TokenStream expressionStream;
			expressionStream.line = first.position.line;
			expressionStream.character = first.position.startPos + formatArgStart;
			expressionStream.fromString(currentExprCode, with->scopeFile->name, errors);
			auto nextLine = expressionStream.next(errors);

			ExpressionResult formatArg = Expression::pushExpression(nextLine, errors, false, this, with);

			auto argType = formatArg.type;
			if (!argType || !argType->sameAs(this))
			{
				if (argType)
				{
					formatArg = formatArg.type->compileToString(formatArg, errors, with);
				}

				if (!formatArg.valid)
				{
					errors->error(ErrorCode::parseInvalidType, first, "Cannot cast type " + Type::toString(argType) + " to string");
				}
			}
			else
			{
				formatArg.code.addBuffer(compileMove(with));
			}
			formatArguments.push_back(formatArg);
			currentExprCode.clear();

			it++;
			continue;
		}

		if (inExpr)
		{
			currentExprCode.push_back(c);
		}
		else
		{
			resultString.push_back(c);
		}
		it++;
	}

	ExpressionResult result = compileStringValue(resultString, with);
	result.code.addBuffer(compileMove(with));
	result.code.addBuffer(varArgs::writeVarArgs(formatArguments));
	result.code.addNew<BytecodeCallNative>("system::string.format");
	result.code.addBuffer(compileEndMove(with));
	return result;
}

ExpressionResult ds::StringType::compileStringValue(std::string str, ParsedScope* with)
{
	ExpressionResult result;

	// Include the null terminator as well
	Size strLength = Size(str.size());
	Size dataSize = strLength + 1;

#if LANG_ALIGN_TYPES_32BIT
	Size misAlign = dataSize % 4;

	if (misAlign)
	{
		dataSize += 4 - misAlign;
	}
#endif

	Size sizeOffset = sizeof(uint32_t);
	Size fullSize = dataSize + sizeOffset;

	Size chunkPos = 0;
	Size remainingSize = dataSize;

	// Push can only push 255 bytes at once, because instruction arguments can
	// at most be 255 bytes long.
	do
	{
		Size chunkSize = std::min(remainingSize, Size(UINT8_MAX));

		remainingSize -= chunkSize;

		BinaryBuffer args;
		args.add((uint8_t*)str.data() + chunkPos, chunkSize);
		result.code.addOperation(BytecodeOp::push, args);

		chunkPos += chunkSize;

	} while (remainingSize > 0);

	result.code.pushInt(strLength);

	result.code.pushInt(fullSize);

	result.code.addNew<BytecodeAllocClass>(this);

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

	BinaryBuffer args;
	args.addValue<uint32_t>(this->size);

	result.code.addOperation(BytecodeOp::copy, args);
	result.code.addBuffer(with->addTemporaryVariable(this));
	return result;
}