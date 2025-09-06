#include <parser/parseScope.hpp>
#include <parser/bytecode/compileBytecodeVariables.hpp>
#include <format>
#include <list>
#include <parser/types/arrayType.hpp>
#include <parser/types/lambdaType.hpp>
using namespace lang;

ExpressionResult lang::ParsedScope::pushExpression(TokenLine& currentLine,
	ErrorContext* errors, bool setExpression, Type* hintType)
{
	ExpressionResult result = getExpressionValue(currentLine, errors, setExpression, hintType);

	// 1 + 1 -> (1, '+'), (1, null)
	// 1 + 1 * 2 -> (1, '+'), (2, '*'), (1, null)
	using exprPart = std::pair<ExpressionResult, std::optional<Token>>;
	std::list<exprPart> expression;
	exprPart* lastElement = &expression.emplace_back(result, std::optional<Token>());

	int32_t highestPriority = INT32_MIN;

	while (!currentLine.empty())
	{
		if (!result.valid)
		{
			return ExpressionResult();
		}
		if (!result.type)
		{
			return result;
		}

		Token operatorToken = currentLine.peek();

		auto exprOperator = stringToOperator(operatorToken.string);

		if (exprOperator == Operator::unknown)
		{
			break;
		}

		highestPriority = std::max(highestPriority, getOperatorPriority(exprOperator));

		lastElement->second = operatorToken;

		currentLine.get();

		auto secondValue = getExpressionValue(currentLine, errors, setExpression, hintType);
		lastElement = &expression.emplace_back(secondValue, std::optional<Token>());
	}

	if (expression.empty())
	{
		return result;
	}

	while (expression.size() > 1)
	{
		int32_t currentPriority = highestPriority;
		highestPriority = INT32_MIN;
		for (auto i = expression.begin(); i != expression.end();)
		{
			if (!i->second)
			{
				break;
			}

			auto op = stringToOperator(i->second->string);
			auto opPriority = getOperatorPriority(op);
			if (opPriority == currentPriority)
			{
				std::list<exprPart>::iterator a = i;
				std::list<exprPart>::iterator b = ++i;
				b->first = compileOperatorBetween(a->first, b->first, op, a->second.value(), errors, true);
				expression.erase(a);
			}
			else
			{
				highestPriority = std::max(highestPriority, opPriority);
				i++;
			}
		}
	}

	return expression.begin()->first;
}

ExpressionResult lang::ParsedScope::getExpressionValue(TokenLine& currentLine, ErrorContext* errors,
	bool setExpression, Type* hintType)
{
	ExpressionResult result = pushValue(currentLine, errors, setExpression, hintType);

	while (!currentLine.empty())
	{
		if (!result.valid)
		{
			return ExpressionResult();
		}
		if (!result.type)
		{
			return result;
		}
		Token nextToken = currentLine.peek();
		if (nextToken == "[")
		{
			currentLine.get();
			auto index = pushExpression(currentLine, errors, false, hintType);

			Type* oldType = result.type;

			result = result.type->compileIndex(result, index, errors, setExpression, this);

			if (!result.valid)
			{
				errors->error(ErrorCode::parseInvalidType, nextToken,
					"Cannot use operator [] with the type " + Type::toString(oldType));
			}

			if (currentLine.get() != "]")
			{
				errors->error(ErrorCode::parseUnexpectedToken, currentLine.previous(),
					"Unexpected '" + currentLine.previous().string + "'");
				break;
			}

			continue;
		}
		if (nextToken == ".")
		{
			currentLine.get();
			auto memberName = currentLine.peek();
			auto oldType = result.type;
			result = result.type->compileMember(result,
				currentLine, errors, setExpression, this);

			if (!result.valid)
			{
				errors->error(ErrorCode::parseUnknowmMember, memberName,
					"The type " + Type::toString(oldType) + " does not contain a member called '" +
						memberName.string + "'");
			}
			continue;
		}
		break;
	}
	return result;
}

ExpressionResult lang::ParsedScope::compileOperatorBetween(ExpressionResult a, ExpressionResult b, Operator op,
	Token opToken, ErrorContext* errors, bool setExpression)
{
	auto oldType = a.type;

	if (op == Operator::equals || op == Operator::notEquals)
	{
		a = a.type->compileEqualsTo(a, b, opToken, errors, this);
		if (op == Operator::notEquals)
		{
			a.code.addOperation(BytecodeOp::boolNot);
		}
	}
	// a <= b is the same as !(b < a) and a >= b is the same thing as !(b > a)
	else if (op == Operator::greaterEquals || op == Operator::lessEquals)
	{
		op = op == Operator::greaterEquals ? Operator::greater : Operator::less;
		a = a.type->compileOperator(op, b, a, this);
		a.code.addOperation(BytecodeOp::boolNot);
	}
	else
	{
		a = a.type->compileOperator(op, a, b, this);
	}
	if (!a.valid)
	{
		errors->error(ErrorCode::parseInvalidType, opToken,
			"The operator '" + opToken.string + "' does not accept types '"
			+ Type::toString(oldType) + "' and '" + Type::toString(b.type) + "'");
	}
	return a;
}

ExpressionResult lang::ParsedScope::pushValue(TokenLine& currentLine,
	ErrorContext* errors, bool setExpression, Type* hintType)
{
	Token value = currentLine.peek();

	if (value == "*")
	{
		currentLine.get();
		auto result = getExpressionValue(currentLine, errors, setExpression, hintType);
		ExpressionResult r;
		return result.type->compileOperator(Operator::dereference, result, r, this);
	}
	if (value == "not")
	{
		currentLine.get();
		auto result = getExpressionValue(currentLine, errors, setExpression, hintType);
		ExpressionResult r;
		return result.type->compileOperator(Operator::logicalNot, result, r, this);
	}

	auto initialPosition = currentLine.savePosition();

	// If the value is a bracket it means it's another expression.
	if (value == "(")
	{
		auto inBraces = currentLine.getInBraces(errors);
		TokenLine line;
		line.lineTokens = &inBraces;
		return pushExpression(line, errors, setExpression, hintType);
	}

	auto [enumValue, enumEntry] = this->scopeFile->getEnum(currentLine);

	if (enumValue)
	{
		ExpressionResult result;
		result.code.pushInt(enumValue->values.at(Token(enumEntry)));
		result.type = IntType::getInstance();
		result.valid = true;
		return result;
	}

	currentLine.get();

	// Allocate a new class
	if (value == "new")
	{
		Token className = currentLine.peek();

		auto foundType = this->scopeFile->getType(currentLine, errors);

		if (!foundType)
		{
			errors->error(ErrorCode::parseUnknownSymbol, value,
				"Unknown class: '" + className.string + "'");
			return ExpressionResult();
		}

		auto compiled = foundType->compileValue(className, currentLine, errors, this, hintType);

		if (compiled.type == nullptr)
		{
			return ExpressionResult();
		}

		compiled.code.addBuffer(compiled.type->compileEndMove(this));
		return compiled;
	}

	// if it's a variable, it's a variable (shocking)
	auto foundVariable = this->variables.find(value);

	if (foundVariable != this->variables.end())
	{
		ExpressionResult result = foundVariable->second.readExpression(this);
		if (setExpression && !foundVariable->second.readOnly)
		{
			auto unrefCode = result.type->compileUnref();

			if (unrefCode.instructions.size())
			{
				result.setCode = result.code;
				result.setCode->addBuffer(unrefCode);
			}
			else
			{
				result.setCode = BytecodeBuffer();
			}

			result.setCode->addBuffer(foundVariable->second.writeValue());
		}
		return result;
	}

	// If this comes after the value, probably a function call
	if (currentLine.peek() == "(")
	{
		Function* function = this->scopeFile->getMethod(value.string);

		if (function)
		{
			auto args = currentLine.getInBraces(errors);

			TokenLine argsLine;
			argsLine.lineTokens = &args;

			auto functionArgs = function->getArguments();

			ExpressionResult callCode = parseFunctionArguments(value, functionArgs,
				argsLine, errors);

			auto fn = function->compileCall();

			if (fn.type)
			{
				fn.code.addBuffer(fn.type->compileEndMove(this));
			}

			callCode.code.addBuffer(fn.code);
			callCode.discardable = fn.discardable;
			callCode.type = fn.type;
			callCode.valid = fn.valid;

			return callCode;
		}
	}

	// Try to convert it into each default type.
	for (auto& i : this->context->defaultTypes)
	{
		size_t pos = currentLine.savePosition();
		auto compiled = i->compileValue(value, currentLine, errors, this, hintType);
		if (compiled.valid)
		{
			return compiled;
		}
		currentLine.loadPosition(pos);
	}

	if (this->inClass)
	{
		currentLine.loadPosition(initialPosition);
		auto result = pushClassValue(currentLine, errors, setExpression);

		if (result.valid)
			return result;
	}

	errors->error(ErrorCode::parseUnknownExpressionType, value, "Unknown symbol: " + value.string);
	return ExpressionResult();
}

ExpressionResult lang::ParsedScope::pushClassValue(TokenLine& currentLine,
	ErrorContext* errors, bool setExpression)
{
	return this->inClass->thisType->compileMember(thisVariable->readExpression(this), currentLine,
		errors, setExpression, this);
}

void lang::ParsedScope::parseSubScope(ParsedFile* file, ErrorContext* errors, std::shared_ptr<BytecodeJumpLabel> breakTarget,
	std::shared_ptr<BytecodeJumpLabel> continueTarget, size_t breakContinueDepth, ScopeOptions options)
{
	ParsedScope conditionScope;
	conditionScope.scopeFunction = options.scopeFunction ? options.scopeFunction : this->scopeFunction;
	conditionScope.code = options.targetBuffer ? options.targetBuffer : this->code;
	conditionScope.tempCounter = this->tempCounter;
	conditionScope.breakTarget = breakTarget;
	conditionScope.continueTarget = continueTarget;
	conditionScope.breakContinueDepth = breakContinueDepth;
	conditionScope.depth = this->depth + 1;
	conditionScope.isLambda = options.isLambda;

	for (auto& i : this->variables)
	{
		if (i.second.ownedBy && !(i.second.isInternal && options.isLambda))
			conditionScope.variables.insert(i);
	}

	TokenStream localStream;
	if (!options.scopeTokens)
	{
		this->tokenStream->getScope(localStream, errors);
		options.scopeTokens = &localStream;
	}

	conditionScope.thisVariable = this->thisVariable;
	conditionScope.variableStackPosition = this->variableStackPosition;
	conditionScope.tokenStream = options.scopeTokens;

	conditionScope.compile(this->context, file, errors);
}

ExpressionResult lang::ParsedScope::parseFunctionArguments(Token functionName, std::vector<FunctionArgument> arguments,
	TokenLine& currentLine, ErrorContext* errors)
{
	ExpressionResult callCode;
	size_t argIndex = 0;

	while (!currentLine.empty())
	{
		auto exprToken = currentLine.peek();

		Type* hintType = arguments.size() > argIndex ? arguments[argIndex].type : nullptr;

		auto expr = this->pushExpression(currentLine, errors, false, hintType);

		if (arguments.size() <= argIndex)
		{
			errors->error(ErrorCode::parseUnexpectedToken, exprToken,
				"Unexpected argument of type " + expr.type->name + " for function '" +
					functionName.string + "'. Only " + std::to_string(arguments.size()) +
					" argument(s) expected.");
			break;
		}

		auto& currentArg = arguments[argIndex++];
		expr.compileToType(exprToken, currentArg.type, this, errors);

		if (!expr.type)
		{
			return ExpressionResult();
		}

		callCode.code.addBuffer(expr.code);
		callCode.code.addBuffer(expr.type->compileMove(this));

		if (currentLine.empty())
			break;
		else if (currentLine.get() != ",")
			errors->error(ErrorCode::parseUnexpectedToken, currentLine.previous(),
				"Expected a ',', got '" + currentLine.previous().string + "'");
	}

	if (argIndex < arguments.size())
	{
		errors->error(ErrorCode::parseUnexpectedToken,
			currentLine.lineTokens->size() ? currentLine.previous() : functionName,
			"Wrong number of arguments for function '" +
				functionName.string + "'. Got " + std::to_string(argIndex) + ", but " + std::to_string(arguments.size()) +
				" argument(s) were expected.");
	}

	return callCode;
}

BytecodeBuffer lang::ParsedScope::addTemporaryVariable(Type* type)
{
	BytecodeBuffer buffer;
	BinaryBuffer args;
	args.addValue<uint32_t>(type->size);
	args.addValue<uint32_t>(0);
	buffer.addOperation(BytecodeOp::storeVariable, args);

	std::string tempName = ".temp_" + std::to_string(tempCounter++);

	auto instruction = std::make_shared<BytecodePushVariable>(tempName, type);

	buffer.add(instruction);

	// clang-format off
	this->variables.insert(
	{Token(tempName), ScopeVariable{
		.name = tempName,
		.variableInstruction = instruction,
		.ownedBy = this,
		.depth = this->depth,
		.type = type,
		.isInternal = true,
	} }).first->second;
	// clang-format on

	this->variableStackPosition += type->size;

	return buffer;
}

void lang::ParsedScope::pushVariableValue(Type* type, bool copy)
{
	BinaryBuffer args;
	// size
	args.addValue<uint32_t>(type->size);
	// stack position (on top so 0)
	args.addValue<uint32_t>(0);
	if (copy)
	{
		code->addBuffer(type->compileMove(this));
	}
	code->addOperation(BytecodeOp::storeVariable, args);
}

ScopeVariable& lang::ParsedScope::addVariable(Token name, Type* type)
{
	auto instruction = std::make_shared<BytecodePushVariable>(name.string, type);
	code->add(instruction);

	auto result = this->variables.insert({ name,
		ScopeVariable{
			.name = name,
			.variableInstruction = instruction,
			.ownedBy = this,
			.depth = this->depth,
			.type = type,
		} });

	this->variableStackPosition += type->size;

	return result.first->second;
}

void lang::ParsedScope::compileScopeExit(size_t toDepth, bool isEnd)
{
	uint32_t size = 0;

	std::vector<Token> toErase;

	for (auto& i : variables)
	{
		if (i.second.depth < toDepth)
		{
			continue;
		}
		if (&i.second != thisVariable || !scopeFunction || scopeFunction->name != "delete")
		{
			auto unrefCode = i.second.type->compileUnref();
			if (unrefCode.instructions.size())
			{
				code->addBuffer(i.second.readValue(this));
				code->addBuffer(unrefCode);
			}

			if (isEnd && &i.second != thisVariable)
			{
				toErase.push_back(i.first);
			}
		}

		size += i.second.type->size;
	}

	for (auto& i : toErase)
	{
		variables.erase(i);
	}

	if (inClass && scopeFunction && scopeFunction->name == "delete")
	{
		code->addBuffer(thisVariable->readValue(this));

		if (inClass->baseDestructor.code.instructions.size())
		{
			code->addBuffer(inClass->baseDestructor.compileCall().code);
		}
		else
		{
			code->addOperation(BytecodeOp::unrefClass);
		}
	}

	if (size)
	{
		code->addNew<BytecodePopVariable>(size, !isEnd);

		this->variableStackPosition -= size;
	}
}

void lang::ParsedScope::setClass(ParsedClass* inClass, bool copy)
{
	this->inClass = inClass;

	if (copy)
	{
		this->code->addBuffer(inClass->thisType->compileMove(this));
	}
	pushVariableValue(inClass->thisType, false);
	this->thisVariable = &addVariable(Token("this"), inClass->thisType);
	this->thisVariable->readOnly = true;
}

void lang::ParsedScope::compile(ParseContext* context, ParsedFile* file, ErrorContext* errors)
{
	this->context = context;
	this->scopeFile = file;

	if (isLambda)
	{
		pushVariableValue(LambdaType::getInstance(), true);
		lambdaVariable = &addVariable(Token(".lambda"), LambdaType::getInstance());
	}

	while (true)
	{
		auto nextLine = this->tokenStream->next(&context->errors);

		if (nextLine.empty())
			break;

		compileLine(nextLine, file, errors);
	}

	if (code->instructions.size() && (*code->instructions.rbegin())->operation == BytecodeOp::ret)
	{
		return;
	}

	compileScopeExit(this->depth, true);
	if (compileReturn)
	{
		if (returnThis)
		{
			code->addBuffer(thisVariable->readValue(this));
		}
		else if (this->scopeFunction && this->scopeFunction->returnType)
		{
			code->addNew<BytecodeCallNative>("system::err::abort");
		}

		code->addOperation(BytecodeOp::ret);
	}
}

void lang::ParsedScope::compileLine(TokenLine line, ParsedFile* file, ErrorContext* errors)
{
	auto first = line.get();

	if (first == "return" && scopeFunction)
	{
		if (scopeFunction->returnType)
		{
			auto expr = pushExpression(line, errors, false, scopeFunction->returnType);

			if (!expr.valid)
			{
				return;
			}
			expr.compileToType(first, scopeFunction->returnType, this, errors);

			this->code->addBuffer(expr.code);
			if (expr.type)
			{
				this->code->addBuffer(expr.type->compileMove(this));
			}
		}
		compileScopeExit(0, false);
		this->code->addOperation(BytecodeOp::ret);
		line.expectEndOfLine(errors);
		return;
	}
	if (first == "if")
	{
		compileIf(line, file, errors);
		return;
	}
	if (first == "for")
	{
		compileFor(line, file, errors);
		return;
	}

	if (first == "break")
	{
		if (this->breakTarget)
		{
			this->compileScopeExit(breakContinueDepth, false);
			this->code->addNew<BytecodeJump>(BytecodeOp::jump, this->breakTarget.get());
		}
		return;
	}

	if (first == "continue")
	{
		if (this->continueTarget)
		{
			this->compileScopeExit(breakContinueDepth, false);
			this->code->addNew<BytecodeJump>(BytecodeOp::jump, this->continueTarget.get());
		}
		return;
	}

	if (first == "while")
	{
		auto conditionLine = line.getUntil("{", errors);

		TokenLine conditionTokens;
		conditionTokens.lineTokens = &conditionLine;

		auto condition = pushExpression(conditionTokens, errors, false, nullptr);
		conditionTokens.expectEndOfLine(errors);
		condition.compileToType(first, BoolType::getInstance(), this, errors);

		auto beginLabel = std::make_shared<BytecodeJumpLabel>("while_begin");
		this->code->add(beginLabel);
		this->code->addBuffer(condition.code);

		auto endLabel = std::make_shared<BytecodeJumpLabel>("while_end");

		this->code->addNew<BytecodeJump>(BytecodeOp::jumpIfNot, endLabel.get());

		parseSubScope(file, errors, endLabel, beginLabel, this->depth + 1);
		this->code->addNew<BytecodeJump>(BytecodeOp::jump, beginLabel.get());

		this->code->add(endLabel);

		return;
	}

	line.position = 0;
	Type* type = file->getType(line, errors);
	// If there is a type, it's probably a variable declaration.
	if (type || first == "var" || first == "const")
	{
		bool isVar = first == "var";
		bool isConst = first == "const";

		if (isVar || isConst)
		{
			// var/const
			line.get();
		}

		Token variableName = line.get();
		variableName.checkIsName(errors);

		BinaryBuffer args;

		Token equals = line.get();
		if (equals == "=")
		{
			auto expr = pushExpression(line, errors, false, type);

			if (isVar || isConst)
			{
				type = expr.type;
			}
			else
			{
				expr.compileToType(equals, type, this, errors);
			}

			code->addBuffer(expr.code);
		}
		else if (!equals.empty())
		{
			errors->error(ErrorCode::parseVarMustHaveInitializer, equals,
				"Expected a '=', got: '" + equals.string + "'");
		}
		else if (isVar || isConst)
		{
			errors->error(ErrorCode::parseVarMustHaveInitializer, variableName,
				"A variable declared with 'var' or 'const' must have an initializer.");
		}
		else
		{
			code->addBuffer(type->defaultValue().code);
		}
		pushVariableValue(type, true);

		addVariable(variableName, type).readOnly = isConst;
		line.expectEndOfLine(errors);
		return;
	}

	line.position = 0;
	auto expr = pushExpression(line, errors, true, nullptr);

	auto compoundOperator = stringToCompoundOperator(line.peek().string);
	if ((line.peek() == "=" || compoundOperator != CompoundOperator::unknown) && expr.valid)
	{
		auto equals = line.get();

		if (!expr.setCode)
		{
			errors->error(ErrorCode::parseReadOnlyValue, line.lineTokens->at(0),
				"This value cannot be written to.");
			return;
		}

		auto valueExpr = pushExpression(line, errors, false, expr.type);
		if (!valueExpr.type)
		{
			return;
		}
		valueExpr.compileToType(equals, expr.type, this, errors);

		if (compoundOperator != CompoundOperator::unknown)
		{
			valueExpr = expr.type->compileOperator(
				Operator(compoundOperator), expr, valueExpr, this);
		}
		valueExpr.code.addBuffer(valueExpr.type->compileMove(this));

		this->code->addBuffer(valueExpr.code);

		this->code->addBuffer(expr.setCode.value());

		line.expectEndOfLine(errors);
	}
	else if (expr.valid)
	{
		this->code->addBuffer(expr.code);

		// Pop the return value from the stack because we're not using it.
		if (expr.type)
		{
			// auto unrefCode = expr.type->compileUnref();
			// if (unrefCode.instructions.size())
			//{
			//	this->code->addBuffer(unrefCode);
			// }
			// else
			{
				BinaryBuffer args;
				args.addValue<uint32_t>(expr.type->size);
				this->code->addOperation(BytecodeOp::pop, args);
			}
			expr.discard(line.lineTokens->at(0), errors);
		}
		line.expectEndOfLine(errors);
	}
	return;
}

void lang::ParsedScope::compileIf(TokenLine line, ParsedFile* file, ErrorContext* errors)
{
	auto conditionLine = line.getUntil("{", errors);

	TokenLine conditionTokens;
	conditionTokens.lineTokens = &conditionLine;

	auto condition = pushExpression(conditionTokens, errors, false, nullptr);
	conditionTokens.expectEndOfLine(errors);
	this->code->addBuffer(condition.code);

	auto endLabel = std::make_shared<BytecodeJumpLabel>("endif");

	this->code->addNew<BytecodeJump>(BytecodeOp::jumpIfNot, endLabel.get());

	parseSubScope(file, errors, this->breakTarget, this->continueTarget, breakContinueDepth);

	auto nextLine = this->tokenStream->peek(errors);

	if (nextLine.peek() == "else")
	{
		this->tokenStream->next(errors);

		nextLine.get();

		auto endElseLabel = std::make_shared<BytecodeJumpLabel>("endElseLabel");
		this->code->addNew<BytecodeJump>(BytecodeOp::jump, endElseLabel.get());

		if (nextLine.peek() == "if")
		{
			nextLine.get();
			this->code->add(endLabel);
			compileIf(nextLine, file, errors);
			this->code->add(endElseLabel);
		}
		else
		{
			this->code->add(endLabel);
			parseSubScope(file, errors, this->breakTarget, this->continueTarget, breakContinueDepth);
			this->code->add(endElseLabel);
		}
	}
	else
	{
		this->code->add(endLabel);
	}
}

void lang::ParsedScope::compileFor(TokenLine line, ParsedFile* file, ErrorContext* errors)
{
	this->depth++;

	// for [...type] [name] in [...expr] {
	//     [block]
	// }
	auto type = file->getType(line, errors);

	bool isVar = false;
	bool isConst = false;

	if (!type)
	{
		auto next = line.get();
		if (next == "var")
		{
			isVar = true;
		}
		else if (next == "const")
		{
			isConst = true;
		}
		else
		{
			errors->error(ErrorCode::parseInvalidType, next, "Invalid type");
			return;
		}
	}

	auto name = line.get();

	auto in = line.peek();
	if (line.expect("in", errors))
	{
		return;
	}

	auto iterLine = line.getUntil("{", errors);

	TokenLine iterTokens;
	iterTokens.lineTokens = &iterLine;

	auto arrayExpression = pushExpression(iterTokens, errors, false, nullptr);
	iterTokens.expectEndOfLine(errors);

	if (!arrayExpression.valid)
	{
		return;
	}

	if (!arrayExpression.type)
	{
		errors->error(ErrorCode::parseUnexpectedToken, in, "Invalid type");
		return;
	}

	auto arrayType = dynamic_cast<ArrayType*>(arrayExpression.type);

	if (!arrayType)
	{
		errors->error(ErrorCode::parseUnexpectedToken, in,
			"For loop expression needs to be an array. Got: " + Type::toString(arrayExpression.type));
		return;
	}

	if (isVar || isConst)
	{
		type = arrayType->baseType;
	}

	this->code->addBuffer(arrayExpression.code);
	pushVariableValue(arrayExpression.type, true);
	auto& iterated = addVariable(Token(".for_iterated"), arrayType);

	auto iterType = IntType::getInstance();

	this->code->addBuffer(iterType->defaultValue().code);

	pushVariableValue(iterType, true);
	auto& iterator = addVariable(Token(".for_iterator"), iterType);

	auto beginLabel = std::make_shared<BytecodeJumpLabel>("for_begin");
	auto continueLabel = std::make_shared<BytecodeJumpLabel>("for_continue");
	this->code->add(beginLabel);

	auto endLabel = std::make_shared<BytecodeJumpLabel>("for_end");
	this->code->addBuffer(arrayType->getLength(iterated.readValue(this)).code);
	this->code->addBuffer(iterator.readValue(this));
	this->code->addOperation(BytecodeOp::greaterInt);

	this->code->addNew<BytecodeJump>(BytecodeOp::jumpIfNot, endLabel.get());

	this->depth++;

	auto index = arrayType->compileIndex(iterated.readExpression(this), iterator.readExpression(this),
		errors, false, this);

	index.compileToType(in, type, this, errors);

	this->code->addBuffer(index.code);

	pushVariableValue(type, true);
	addVariable(name, type);

	parseSubScope(file, errors, endLabel, continueLabel, this->depth);


	this->code->add(continueLabel);
	compileScopeExit(this->depth, true);
	this->code->addBuffer(iterator.readValue(this));
	this->code->pushInt(1);
	this->code->addOperation(BytecodeOp::addInt);
	this->code->addBuffer(iterator.writeValue());

	this->code->addNew<BytecodeJump>(BytecodeOp::jump, beginLabel.get());
	this->code->add(endLabel);

	this->depth -= 2;
	compileScopeExit(this->depth + 1, true);
}

BytecodeBuffer lang::ScopeVariable::readValue(ParsedScope* with) const
{
	BytecodeBuffer result;
	if (with->isLambda && ownedBy != with)
	{
		bool found = false;

		for (auto& i : with->scopeFunction->capturedVariables)
		{
			if (i.string == this->name.string)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			with->scopeFunction->capturedVariables.push_back(name);
			this->lambdaOffset = with->lambdaOffset;
			with->lambdaOffset += type->size;
		}
		result.addBuffer(with->lambdaVariable->readValue(with));
		result.pushInt(this->lambdaOffset + 4);
		result.pushInt(type->size);
		result.addOperation(BytecodeOp::classMember);
		return result;
	}

	result.addNew<BytecodeReadVariable>(this->variableInstruction);
	return result;
}

BytecodeBuffer lang::ScopeVariable::writeValue() const
{
	BytecodeBuffer result;
	result.addNew<BytecodeStoreVariable>(this->variableInstruction);
	return result;
}

ExpressionResult lang::ScopeVariable::readExpression(ParsedScope* with) const
{
	ExpressionResult result;
	result.valid = true;
	result.type = this->type;
	result.code = readValue(with);
	return result;
}
