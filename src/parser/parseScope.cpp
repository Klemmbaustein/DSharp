#include <parser/parseScope.hpp>
#include <parser/bytecode/compileBytecodeVariables.hpp>
#include <format>
#include <list>
#include <optional>
#include <print>
using namespace lang;

ExpressionResult lang::ParsedScope::pushExpression(TokenLine& currentLine,
	ErrorContext* errors, bool setExpression)
{
	ExpressionResult result = getExpressionValue(currentLine, errors, setExpression);

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

		auto secondValue = getExpressionValue(currentLine, errors, setExpression);
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
	bool setExpression)
{
	ExpressionResult result = pushValue(currentLine, errors, setExpression);

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
			auto index = pushExpression(currentLine, errors, false);

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
	result.valid = true;
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
			std::format("The operator '{}' does not accept types '{}' and '{}'",
				opToken.string, oldType->name, b.type->name));
	}
	return a;
}

ExpressionResult lang::ParsedScope::pushValue(TokenLine& currentLine,
	ErrorContext* errors, bool setExpression)
{
	Token value = currentLine.peek();

	if (value == "*")
	{
		currentLine.get();
		auto result = pushValue(currentLine, errors, setExpression);
		ExpressionResult r;
		return result.type->compileOperator(Operator::dereference, result, r, this);
	}
	if (value == "not")
	{
		currentLine.get();
		auto result = pushValue(currentLine, errors, setExpression);
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
		return pushExpression(line, errors, setExpression);
	}

	currentLine.get();

	// Allocate a new class
	if (value == "new")
	{
		Token className = currentLine.peek();

		auto foundType = this->scopeFile->getType(currentLine);

		if (!foundType)
		{
			errors->error(ErrorCode::parseUnknownSymbol, value,
				"Unknown class: '" + className.string + "'");
			return ExpressionResult();
		}

		auto compiled = foundType->compileValue(className, currentLine, errors, this);

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
		ExpressionResult result;
		result.valid = true;
		result.type = foundVariable->second.type;
		result.code = foundVariable->second.readValue(this);
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

			result.setCode->addBuffer(foundVariable->second.writeValue(this));
		}
		return result;
	}

	// If this comes after the value, probably a function call
	if (currentLine.peek() == "(")
	{
		Function* function = this->scopeFile->getMethod(value.string);

		if (!function && !this->inClass)
		{
			errors->error(ErrorCode::parseUnknownSymbol, value,
				"Unknown function: '" + value.string + "'");
			return ExpressionResult();
		}
		else if (function)
		{
			auto args = currentLine.getInBraces(errors);

			TokenLine argsLine;
			argsLine.lineTokens = &args;

			auto functionArgs = function->getArguments();

			ExpressionResult callCode = parseFunctionArguments(function->getFullName(), functionArgs,
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
		auto compiled = i->compileValue(value, currentLine, errors, this);
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
	ExpressionResult result;
	result.valid = true;
	result.type = this->inClass->thisType;
	result.code = thisVariable->readValue(this);

	result = this->inClass->thisType->compileMember(result, currentLine,
		errors, setExpression, this);
	return result;
}

void lang::ParsedScope::parseSubScope(ParsedFile* file, ErrorContext* errors,
	BytecodeJumpLabel* breakTarget, BytecodeJumpLabel* continueTarget, size_t breakContinueDepth)
{
	ParsedScope conditionScope;
	TokenStream conditionTokens;
	conditionScope.scopeFunction = this->scopeFunction;
	conditionScope.tokenStream = &conditionTokens;
	conditionScope.code = this->code;
	conditionScope.tempCounter = this->tempCounter;
	conditionScope.breakTarget = breakTarget;
	conditionScope.continueTarget = continueTarget;
	conditionScope.breakContinueDepth = breakContinueDepth;
	conditionScope.depth = this->depth + 1;

	for (auto& i : this->variables)
	{
		if (i.second.ownedBy)
			conditionScope.variables.insert(i);
	}

	conditionScope.thisVariable = this->thisVariable;
	conditionScope.variableStackPosition = this->variableStackPosition;
	this->tokenStream->getScope(conditionTokens, errors);

	conditionScope.compile(this->context, file, errors);
}

ExpressionResult lang::ParsedScope::parseFunctionArguments(std::string functionName, std::vector<FunctionArgument> arguments,
	TokenLine& currentLine, ErrorContext* errors)
{
	ExpressionResult callCode;
	size_t argIndex = 0;

	while (!currentLine.empty())
	{
		auto exprToken = currentLine.peek();
		auto expr = this->pushExpression(currentLine, errors, false);

		if (arguments.size() <= argIndex)
		{
			errors->error(ErrorCode::parseUnexpectedToken, exprToken,
				"Unexpected argument of type " + expr.type->name + " for function '" +
					functionName + "'. Only " + std::to_string(arguments.size()) +
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

	auto instruction = new BytecodePushVariable(tempName, type);

	buffer.add(instruction);

	// clang-format off
	auto& result = this->variables.insert(
	{Token(tempName), ScopeVariable{
		.name = tempName,
		.variableInstruction = instruction,
		.ownedBy = this,
		.depth = this->depth,
		.type = type,
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

ParsedScope::ScopeVariable& lang::ParsedScope::addVariable(Token name, Type* type)
{
	auto instruction = new BytecodePushVariable(name.string, type);
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
		}

		size += i.second.type->size;
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
		code->add(new BytecodePopVariable(size, !isEnd));

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
			code->add(new BytecodeCallNative("system::err::abort"));
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
			auto expr = pushExpression(line, errors, false);

			if (!expr.type)
			{
				return;
			}
			expr.compileToType(first, scopeFunction->returnType, this, errors);

			this->code->addBuffer(expr.code);
			this->code->addBuffer(expr.type->compileMove(this));
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

	if (first == "break")
	{
		if (this->breakTarget)
		{
			this->compileScopeExit(breakContinueDepth, false);
			this->code->add(new BytecodeJump(BytecodeOp::jump, this->breakTarget));
		}
		return;
	}

	if (first == "continue")
	{
		if (this->continueTarget)
		{
			this->compileScopeExit(breakContinueDepth, false);
			this->code->add(new BytecodeJump(BytecodeOp::jump, this->continueTarget));
		}
		return;
	}

	if (first == "while")
	{
		auto conditionLine = line.getUntil("{", errors);

		TokenLine conditionTokens;
		conditionTokens.lineTokens = &conditionLine;

		auto condition = pushExpression(conditionTokens, errors, false);
		conditionTokens.expectEndOfLine(errors);

		auto beginLabel = new BytecodeJumpLabel("while_begin");
		this->code->add(beginLabel);
		this->code->addBuffer(condition.code);

		auto endLabel = new BytecodeJumpLabel("while_end");

		this->code->add(new BytecodeJump(BytecodeOp::jumpIfNot, endLabel));

		parseSubScope(file, errors, endLabel, beginLabel, this->depth + 1);
		this->code->add(new BytecodeJump(BytecodeOp::jump, beginLabel));

		this->code->add(endLabel);

		return;
	}

	line.position = 0;
	Type* type = file->getType(line);
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
			auto expr = pushExpression(line, errors, false);

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
	auto expr = pushExpression(line, errors, true);

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

		auto valueExpr = pushExpression(line, errors, false);
		if (!valueExpr.type)
		{
			return;
		}
		valueExpr.compileToType(equals, expr.type, this, errors);
		valueExpr.code.addBuffer(valueExpr.type->compileMove(this));

		if (compoundOperator != CompoundOperator::unknown)
		{
			valueExpr = expr.type->compileOperator(
				Operator(compoundOperator), expr, valueExpr, this);
		}

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

	auto condition = pushExpression(conditionTokens, errors, false);
	conditionTokens.expectEndOfLine(errors);
	this->code->addBuffer(condition.code);

	auto endLabel = new BytecodeJumpLabel("endif");

	this->code->add(new BytecodeJump(BytecodeOp::jumpIfNot, endLabel));

	parseSubScope(file, errors, this->breakTarget, this->continueTarget, breakContinueDepth);

	auto nextLine = this->tokenStream->peek(errors);

	if (nextLine.peek() == "else")
	{
		this->tokenStream->next(errors);

		nextLine.get();

		auto endElseLabel = new BytecodeJumpLabel("endElseLabel");
		this->code->add(new BytecodeJump(BytecodeOp::jump, endElseLabel));

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

BytecodeBuffer lang::ParsedScope::ScopeVariable::readValue(ParsedScope* scope) const
{
	BytecodeBuffer result;
	result.add(new BytecodeReadVariable(this->variableInstruction));
	return result;
}

BytecodeBuffer lang::ParsedScope::ScopeVariable::writeValue(ParsedScope* scope) const
{
	BytecodeBuffer result;
	result.add(new BytecodeStoreVariable(this->variableInstruction));
	return result;
}
