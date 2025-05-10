#include <parser/parseScope.hpp>
#include <modules/system.hpp>
#include <parser/compileBytecodeVariables.hpp>
using namespace lang;

ExpressionResult lang::ParsedScope::pushExpression(TokenLine& currentLine, ErrorContext* errors, bool setExpression)
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

		result = result.type->compileOperator(exprOperator, result, secondValue);
	}

	result.valid = true;
	return result;
}

ExpressionResult lang::ParsedScope::pushValue(TokenLine& currentLine, ErrorContext* errors, bool setExpression)
{
	Token value = currentLine.peek();

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
		if (setExpression)
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

			while (!argsLine.empty())
			{
				auto expr = this->pushExpression(argsLine, errors, false);

				this->code->addBuffer(expr.code);

				if (argsLine.empty())
					break;
				else if (argsLine.get() != ",")
					errors->error(ErrorCode::parseUnexpectedToken, argsLine.previous(),
						"Expected a ',', got '" + argsLine.previous().string + "'");
			}

			auto fn = function->compileCall();

			if (fn.type)
			{
				fn.code.addBuffer(fn.type->compileEndMove(this));
			}
			return fn;
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
		currentLine.position = 0;
		auto result = pushClassValue(currentLine, errors, setExpression);

		if (result.valid)
			return result;
	}

	errors->error(ErrorCode::parseUnknownExpressionType, value, "Unknown symbol: " + value.string);
	return ExpressionResult();
}

ExpressionResult lang::ParsedScope::pushClassValue(TokenLine& currentLine, ErrorContext* errors, bool setExpression)
{
	ExpressionResult result;
	result.valid = true;
	result.type = this->inClass->thisType;
	result.code = thisVariable->readValue(this);

	result = this->inClass->thisType->compileMember(result, currentLine, errors, setExpression, this);
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

BytecodeBuffer lang::ParsedScope::addTemporaryVariable(Type* type)
{
	BytecodeBuffer buffer;
	BinaryBuffer args;
	args.addValue<uint32_t>(type->size);
	args.addValue<uint32_t>(0);
	buffer.addOperation(BytecodeOp::storeVariable, args);

	std::string tempName = ".temp_" + std::to_string(tempCounter++);

	auto instruction = new BytecodePushVariable(tempName, type);

	code->add(instruction);

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
		code->addBuffer(type->compileEndMove(this));
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
		code->addOperation(BytecodeOp::unrefClass);
	}

	if (size)
	{
		code->add(new BytecodePopVariable(size));

		this->variableStackPosition -= size;
	}
}

void lang::ParsedScope::setClass(ParsedClass* inClass)
{
	this->inClass = inClass;

	pushVariableValue(inClass->thisType, !this->scopeFunction || this->scopeFunction->name != "delete");
	this->thisVariable = &addVariable(Token("this"), inClass->thisType);
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
		auto condition = pushExpression(line, errors, false);

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
	if (type || first == "var")
	{
		bool isVar = first == "var";

		if (isVar)
		{
			// var
			line.get();
		}

		Token variableName = line.get();

		BinaryBuffer args;
		if (line.get() == "=")
		{
			auto expr = pushExpression(line, errors, false);

			if (isVar)
			{
				type = expr.type;
			}
			else
			{
				expr.compileToType(line.previous(), type, errors);
			}

			code->addBuffer(expr.code);
			pushVariableValue(expr.type, true);
		}
		else if (isVar)
		{
			errors->error(ErrorCode::parseVarMustHaveInitializer, variableName,
				"A variable declared with 'var' must have an initializer.");
		}

		addVariable(variableName, type);
		line.expectEndOfLine(errors);
		return;
	}

	line.position = 0;
	auto expr = pushExpression(line, errors, true);

	if (line.peek() == "=" && expr.valid)
	{
		// Reparse the expression but set the read flag to true.
		auto equals = line.get();

		if (!expr.setCode)
		{
			errors->error(ErrorCode::parseReadOnlyValue, line.lineTokens->at(0),
				"This value cannot be written to.");
			return;
		}

		auto valueExpr = pushExpression(line, errors, false);
		valueExpr.compileToType(equals, expr.type, errors);
		this->code->addBuffer(valueExpr.code);

		auto moveCode = expr.type->compileEndMove(this);
		if (moveCode.instructions.size())
		{
			this->code->addBuffer(moveCode);
		}

		this->code->addBuffer(expr.setCode.value());
		line.expectEndOfLine(errors);
	}
	else if (expr.valid)
	{
		this->code->addBuffer(expr.code);

		// Pop the return value from the stack because we're not using it.
		if (expr.type)
		{
			BinaryBuffer args;
			args.addValue<uint32_t>(expr.type->size);
			this->code->addOperation(BytecodeOp::pop, args);
			expr.discard(line.lineTokens->at(0), errors);
		}
		line.expectEndOfLine(errors);
	}
	return;
}

void lang::ParsedScope::compileIf(TokenLine line, ParsedFile* file, ErrorContext* errors)
{
	auto condition = pushExpression(line, errors, false);
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
