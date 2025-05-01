#include <parser/parseScope.hpp>
#include <modules/system.hpp>
using namespace lang;

ExpressionResult lang::ParsedScope::pushExpression(TokenLine& currentLine, ErrorContext* errors, bool setExpression)
{
	ExpressionResult result = pushValue(currentLine, errors, setExpression);

	while (!currentLine.empty())
	{
		if (result.type == nullptr)
		{
			return ExpressionResult();
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
			result = result.type->compileMember(result, currentLine, errors, setExpression);
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

	return result;
}

ExpressionResult lang::ParsedScope::pushValue(TokenLine& currentLine, ErrorContext* errors, bool setExpression)
{
	Token value = currentLine.peek();
	ExpressionResult result;

	// If the value is a bracket it means it's another expression.
	if (value == "(")
	{
		auto inBraces = currentLine.getInBraces();
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
			errors->error(ErrorCode::parseUnknownSymbol, value, "Unknown class: '" + className.string + "'");
			return ExpressionResult();
		}

		return foundType->compileValue(className, currentLine);
	}

	// if it's a variable, it's a variable (shocking)
	auto foundVariable = this->variables.find(value);

	if (foundVariable != this->variables.end())
	{
		ExpressionResult result;
		result.valid = true;
		result.type = foundVariable->second.type;
		if (setExpression)
		{
			result.setCode = foundVariable->second.writeValue(this);
		}
		result.code = foundVariable->second.readValue(this);
		return result;
	}

	// If this comes after the value, probably a function call
	if (currentLine.peek() == "(")
	{
		Function* function = this->scopeFile->getMethod(value.string);

		if (!function)
		{
			errors->error(ErrorCode::parseUnknownSymbol, value, "Unknown function: '" + value.string + "'");
			return ExpressionResult();
		}

		auto args = currentLine.getInBraces();

		if (!args.empty())
		{
			TokenLine arguments;
			arguments.lineTokens = &args;
			auto expr = this->pushExpression(arguments, errors, false);

			this->code->addBuffer(expr.code);
		}

		return function->compileCall();
	}

	// Try to convert it into each default type.
	for (auto& i : this->context->defaultTypes)
	{
		size_t pos = currentLine.savePosition();
		auto compiled = i->compileValue(value, currentLine);
		if (compiled.valid)
		{
			return compiled;
		}
		currentLine.loadPosition(pos);
	}
	errors->error(ErrorCode::parseUnknownExpressionType, value, "Unknown symbol: " + value.string);
	return ExpressionResult();
}

void lang::ParsedScope::pushVariableValue(Type* type)
{
	BinaryBuffer args;
	// size
	args.addValue<uint32_t>(type->size);
	// stack position (on top so 0)
	args.addValue<uint32_t>(0);
	code->addOperation(BytecodeOp::storeVariable, args);
}

ParsedScope::ScopeVariable& lang::ParsedScope::addVariable(Token name, Type* type)
{
	BinaryBuffer args;
	args.addValue<uint32_t>(type->size);
	code->addOperation(BytecodeOp::pushVariable, args);

	auto result = this->variables.insert({ name,
		ScopeVariable{
			.name = name,
			.stackPosition = this->variableStackPosition,
			.ownedBy = this,
			.type = type,
		} });

	this->variableStackPosition += type->size;

	return result.first->second;
}

void lang::ParsedScope::compileScopeExit(bool full)
{
	for (auto& i : variables)
	{
		if (i.second.ownedBy == this || full)
		{
			BinaryBuffer args;
			args.addValue<uint32_t>(i.second.type->size);
			code->addOperation(BytecodeOp::popVariable, args);

			this->variableStackPosition -= i.second.type->size;
		}
	}
}

void lang::ParsedScope::compile(ParseContext* context, ParsedFile* file, ErrorContext* errors)
{
	this->context = context;
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
	code->addOperation(BytecodeOp::ret);
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
		}
		compileScopeExit(true);
		this->code->addOperation(BytecodeOp::ret);
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
			pushVariableValue(expr.type);
		}
		else if (isVar)
		{
			errors->error(ErrorCode::parseVarMustHaveInitializer, variableName, "A variable declared with 'var' must have an initializer.");
		}

		addVariable(variableName, type);

		return;
	}

	line.position = 0;
	auto expr = pushExpression(line, errors, false);

	if (line.peek() == "=")
	{
		// Reparse the expression but set the read flag to true.
		line.position = 0;
		auto setValueExpr = pushExpression(line, errors, true);

		auto equals = line.get();

		if (equals != "=")
		{
			errors->error(ErrorCode::internalError, line.previous(), "Read and write expressions were parsed differently. This should never happen.");
			return;
		}

		if (!setValueExpr.setCode)
		{
			errors->error(ErrorCode::parseReadOnlyValue, line.lineTokens->at(0), "This value cannot be written to.");
			return;
		}

		auto valueExpr = pushExpression(line, errors, false);
		valueExpr.compileToType(equals, setValueExpr.type, errors);
		this->code->addBuffer(valueExpr.code);
		this->code->addBuffer(setValueExpr.setCode.value());
	}
	else
	{
		this->code->addBuffer(expr.code);

		// Pop the return value from the stack because we're not using it.
		if (expr.type)
		{
			BinaryBuffer args;
			args.addValue<uint32_t>(expr.type->size);
			this->code->addOperation(BytecodeOp::pop, args);
		}
		expr.discard(line.lineTokens->at(0), errors);
	}
	return;
}

uint32_t lang::ParsedScope::ScopeVariable::getRelativePosition(ParsedScope* scope) const
{
	return scope->variableStackPosition - this->stackPosition;
}

BytecodeBuffer lang::ParsedScope::ScopeVariable::readValue(ParsedScope* scope) const
{
	BytecodeBuffer result;
	BinaryBuffer args;

	// Size
	args.addValue(this->type->size);
	// stack position
	args.addValue(getRelativePosition(scope));

	result.addOperation(BytecodeOp::readVariable, args);

	return result;
}

BytecodeBuffer lang::ParsedScope::ScopeVariable::writeValue(ParsedScope* scope) const
{
	BytecodeBuffer result;
	BinaryBuffer args;

	// Size
	args.addValue(this->type->size);
	// stack position
	args.addValue(getRelativePosition(scope));

	result.addOperation(BytecodeOp::storeVariable, args);

	return result;
}
