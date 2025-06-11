#include <parser/parseScope.hpp>
#include <parser/bytecode/compileBytecodeVariables.hpp>
#include <format>
using namespace lang;

ExpressionResult lang::ParsedScope::pushExpression(TokenLine& currentLine,
	ErrorContext* errors, bool setExpression)
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

		Token operatorToken = currentLine.peek();

		if (operatorToken == "[")
		{
			currentLine.get();
			auto index = pushExpression(currentLine, errors, false);

			result = result.type->compileIndex(result, index, errors, setExpression, this);

			if (currentLine.get() != "]")
			{
				errors->error(ErrorCode::parseUnexpectedToken, currentLine.previous(),
					"Unexpected '" + currentLine.previous().string + "'");
				break;
			}

			continue;
		}

		auto exprOperator = stringToOperator(operatorToken.string);
		if (exprOperator == Operator::unknown)
		{
			break;
		}

		currentLine.get();

		if (exprOperator == Operator::member)
		{
			result = result.type->compileMember(result, currentLine, errors, setExpression, this);
			continue;
		}

		auto secondValue = pushValue(currentLine, errors, setExpression);

		// if (!nextOperatorToken.empty())
		//{
		//	auto nextOperator = stringToOperator(nextOperatorToken.string);

		//	if (operatorHasPriority(nextOperator, exprOperator))
		//	{
		//		currentLine.get();

		//		auto firstValue = result;

		//		auto thirdValue = pushValue(currentLine, errors, willDiscard);
		//		result = result.type->compileOperator(nextOperator, secondValue, thirdValue);
		//		result = result.type->compileOperator(exprOperator, result, firstValue);
		//		continue;
		//	}
		//}

		auto oldType = result.type;

		if (exprOperator == Operator::equals || exprOperator == Operator::notEquals)
		{
			result = result.type->compileEqualsTo(result, secondValue);
			if (exprOperator == Operator::notEquals)
			{
				result.code.addOperation(BytecodeOp::boolNot);
			}
		}
		// a <= b is the same as !(b < a) and a >= b is the same thing as !(b > a)
		else if (exprOperator == Operator::greaterEquals || exprOperator == Operator::lessEquals)
		{
			exprOperator = exprOperator == Operator::greaterEquals ? Operator::greater : Operator::less;
			result = result.type->compileOperator(exprOperator, secondValue, result, this);
			result.code.addOperation(BytecodeOp::boolNot);
		}
		else
		{
			result = result.type->compileOperator(exprOperator, result, secondValue, this);
		}
		if (!result.valid)
		{
			errors->error(ErrorCode::parseInvalidType, operatorToken,
				std::format("The operator '{}' does not accept types '{}' and '{}'",
					operatorToken.string, oldType->name, secondValue.type->name));
			break;
		}
	}

	result.valid = true;
	return result;
}

ExpressionResult lang::ParsedScope::pushValue(TokenLine& currentLine,
	ErrorContext* errors, bool setExpression)
{
	Token value = currentLine.peek();

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

			ExpressionResult callCode = parseFunctionArguments(function->getFullName(), functionArgs, argsLine, errors);

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

void lang::ParsedScope::parseSubScope(ParsedFile* file, ErrorContext* errors)
{
	ParsedScope conditionScope;
	TokenStream conditionTokens;
	conditionScope.scopeFunction = this->scopeFunction;
	conditionScope.tokenStream = &conditionTokens;
	conditionScope.code = this->code;
	conditionScope.tempCounter = this->tempCounter;

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
		expr.compileToType(exprToken, currentArg.type, errors);

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
			.type = type,
		} });

	this->variableStackPosition += type->size;

	return result.first->second;
}

void lang::ParsedScope::compileScopeExit(bool full)
{
	uint32_t size = 0;
	for (auto& i : variables)
	{
		if ((i.second.ownedBy && i.second.ownedBy != this) && !full)
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
		code->add(new BytecodePopVariable(size));

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

	compileScopeExit(false);
	if (compileReturn)
	{
		if (returnThis)
		{
			code->addBuffer(thisVariable->readValue(this));
		}

		code->addOperation(BytecodeOp::ret);
	}
}

void lang::ParsedScope::compileLine(TokenLine line, ParsedFile* file, ErrorContext* errors)
{
	auto first = line.get();

	if (first.string == "return" && scopeFunction)
	{
		if (scopeFunction->returnType)
		{
			auto expr = pushExpression(line, errors, false);

			expr.compileToType(first, scopeFunction->returnType, errors);

			this->code->addBuffer(expr.code);
			this->code->addBuffer(expr.type->compileMove(this));
		}
		compileScopeExit(true);
		this->code->addOperation(BytecodeOp::ret);
		line.expectEndOfLine(errors);
		return;
	}
	if (first.string == "if")
	{
		compileIf(line, file, errors);
		return;
	}

	if (first.string == "while")
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

		parseSubScope(file, errors);
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
				expr.compileToType(equals, type, errors);
			}

			code->addBuffer(expr.code);
			pushVariableValue(expr.type, true);
		}
		else if (isVar || isConst)
		{
			errors->error(ErrorCode::parseVarMustHaveInitializer, variableName,
				"A variable declared with 'var' or 'const' must have an initializer.");
		}

		addVariable(variableName, type);
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
		valueExpr.compileToType(equals, expr.type, errors);
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
			//auto unrefCode = expr.type->compileUnref();
			//if (unrefCode.instructions.size())
			//{
			//	this->code->addBuffer(unrefCode);
			//}
			//else
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

	parseSubScope(file, errors);

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
			parseSubScope(file, errors);
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
