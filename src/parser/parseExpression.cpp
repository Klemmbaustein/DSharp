#include <ds/parser/parseExpression.hpp>
#include <ds/service/languageService.hpp>
#include <ds/parser/types/taskType.hpp>

using namespace ds;

ExpressionResult ds::Expression::pushExpression(TokenLine& currentLine,
	ErrorContext* errors, bool setExpression, Type* hintType, ParsedScope* scope)
{
	ExpressionResult result = getExpressionValue(currentLine, errors, setExpression, hintType, scope);

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

		auto secondValue = getExpressionValue(currentLine, errors, setExpression, hintType, scope);
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
				b->first = compileOperatorBetween(a->first, b->first, op, a->second.value(),
					errors, true, scope);
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

ExpressionResult ds::Expression::getExpressionValue(TokenLine& currentLine, ErrorContext* errors,
	bool setExpression, Type* hintType, ParsedScope* scope)
{
	ExpressionResult result = pushValue(currentLine, errors, setExpression, hintType, scope);

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
		const Token& nextToken = currentLine.peek();
		if (nextToken == "[")
		{
			currentLine.get();
			auto index = pushExpression(currentLine, errors, false, hintType, scope);
			if (!index.valid)
			{
				break;
			}

			Type* oldType = result.type;

			result = result.type->compileIndex(result, index, errors, setExpression, scope);

			if (!result.valid)
			{
				errors->error(ErrorCode::parseInvalidType, nextToken,
					"Cannot use operator [] with the type " + Type::toString(oldType));
				break;
			}

			if (currentLine.expect("]", errors))
			{
				break;
			}

			continue;
		}
		if (nextToken == ".")
		{
			currentLine.get();
			auto& memberName = currentLine.peek();
			auto oldType = result.type;
			result = result.type->compileMember(result,
				currentLine, errors, setExpression, scope);

			if (!result.valid)
			{
				errors->error(ErrorCode::parseUnknownMember, memberName,
					"The type " + Type::toString(oldType) + " does not have a member called '" +
						memberName.string + "'");
			}
			continue;
		}
		else if (nextToken == "is")
		{
			currentLine.get();

			bool isNot = currentLine.peek() == "not";

			if (isNot)
			{
				currentLine.get();
			}

			Type* foundType = scope->scopeFile->getType(currentLine, errors);

			ClassType* foundClass = foundType ? foundType->asClass() : nullptr;

			if (!foundClass)
			{
				errors->error(ErrorCode::parseInvalidType, nextToken,
					"Type " + Type::toString(foundType) + " is not a class");
				return result;
			}
			ClassType* expressionClass = result.type ? result.type->asClass() : nullptr;

			if (!expressionClass)
			{
				errors->error(ErrorCode::parseInvalidType, nextToken,
					"Type " + Type::toString(expressionClass) + " is not a class");
				return result;
			}

			if (!foundClass->isInterface && !foundClass->isSubclassOf(expressionClass))
			{
				errors->error(ErrorCode::parseInvalidType, nextToken,
					"Class " + Type::toString(foundClass) + " is not a subclass of " + Type::toString(expressionClass));
			}

			if (!scope->context->service)
			{
				BinaryBuffer args;
				args.addValue(foundType->id);

				result.code.addOperation(BytecodeOp::classIs, args);
				if (isNot)
				{
					result.code.addOperation(BytecodeOp::boolNot, args);
				}
			}
			result.type = scope->context->registry->getEntry<BoolType>();
			continue;
		}
		else if (nextToken == "as")
		{
			currentLine.get();

			Type* foundType = scope->scopeFile->getType(currentLine, errors);

			bool isNullable = foundType && typeid(*foundType) == typeid(NullableClassType);
			ClassType* foundClass = foundType ? foundType->asClass() : nullptr;

			ClassType* expressionClass = result.type ? result.type->asClass() : nullptr;

			if (!foundClass || !expressionClass)
			{
				if (foundType)
				{
					result.compileToType(nextToken, foundType, scope, errors);
				}
				else
				{
					errors->error(ErrorCode::parseInvalidType, nextToken,
						"Type " + Type::toString(foundType) + " is not a class");
				}
				return result;
			}

			if (!expressionClass)
			{
				errors->error(ErrorCode::parseInvalidType, nextToken,
					"Type " + Type::toString(expressionClass) + " is not a class");
				return result;
			}

			if (!foundClass->isInterface && !foundClass->isSubclassOf(expressionClass))
			{
				errors->error(ErrorCode::parseInvalidType, nextToken,
					"Class " + Type::toString(foundClass) + " is not a subclass of " + Type::toString(expressionClass));
			}

			BinaryBuffer args;
			args.addValue(foundType->id);
			args.addValue<Bool>(isNullable);
			result.code.addOperation(BytecodeOp::classAs, args);
			result.type = foundType;
			continue;
		}
		break;
	}
	return result;
}

ExpressionResult ds::Expression::compileOperatorBetween(ExpressionResult a, ExpressionResult b, Operator op,
	Token opToken, ErrorContext* errors, bool setExpression, ParsedScope* scope)
{
	auto oldType = a.type;

	if (!a.type || !b.type)
	{
		return ExpressionResult();
	}

	if (op == Operator::equals || op == Operator::notEquals)
	{
		a = a.type->compileEqualsTo(a, b, opToken, errors, scope);
		if (op == Operator::notEquals)
		{
			a.code.addOperation(BytecodeOp::boolNot);
		}
	}
	// a <= b is the same as !(b < a) and a >= b is the same thing as !(b > a)
	else if (op == Operator::greaterEquals || op == Operator::lessEquals)
	{
		op = op == Operator::greaterEquals ? Operator::greater : Operator::less;
		a = a.type->compileOperator(op, b, a, scope);
		a.code.addOperation(BytecodeOp::boolNot);
	}
	else if (a.type)
	{
		a = a.type->compileOperator(op, a, b, scope);
	}
	if (!a.valid)
	{
		errors->error(ErrorCode::parseInvalidType, opToken,
			"The operator '" + opToken.string + "' does not accept types '" + Type::toString(oldType) + "' and '" + Type::toString(b.type) + "'");
	}
	return a;
}

ExpressionResult ds::Expression::pushValue(TokenLine& currentLine,
	ErrorContext* errors, bool setExpression, Type* hintType, ParsedScope* scope)
{
	Token value = currentLine.peek();

	auto op = stringToUnaryOperator(value.string);

	if (op != Operator::unknown)
	{
		currentLine.get();
		auto result = getExpressionValue(currentLine, errors, setExpression, hintType, scope);
		if (result.type && result.valid)
		{
			ExpressionResult r;
			r = result.type->compileOperator(op, result, r, scope);
			if (!r.valid)
			{
				errors->error(ErrorCode::parseInvalidType, value,
					"The operator '" + value.string + "' does not accept the type '" + Type::toString(result.type) + "'");
			}
			return r;
		}
	}

	auto initialPosition = currentLine.savePosition();

	// If the value is a bracket it means it's another expression.
	if (value == "(")
	{
		auto inBraces = currentLine.getInBraces(errors);
		TokenLine line;
		line.lineTokens = &inBraces;
		return pushExpression(line, errors, setExpression, hintType, scope);
	}

	auto [enumValue, enumEntry] = scope->scopeFile->getEnum(currentLine);

	if (enumValue)
	{
		ExpressionResult result;

		auto found = enumValue->values.find(Token(enumEntry));

		if (found != enumValue->values.end())
		{
			result.code.pushInt(found->second);
			// #ifdef WITH_LANGUAGE_SERVICE
			//			if (scope->context->service)
			//			{
			//				scope->context->service->files[scope->scopeFile->name].variables.push_back(value);
			//			}
			// #endif
			result.type = scope->context->registry->getEntry<IntType>();
			result.valid = true;
		}
		else
		{
			errors->error(ErrorCode::parseUnexpectedToken, currentLine.previous(),
				"The enum '" + enumValue->name + "' doesn't contain an entry '" + enumEntry + "'");
			result.valid = false;
		}
		return result;
	}

	currentLine.get();

	// Allocate a new class
	if (value == "new")
	{
		Token className = currentLine.peek();

		Type* foundType = nullptr;

		if (className == "(" && hintType)
		{
			foundType = hintType->asClass();
			className = value;
		}
		else
		{
			foundType = scope->scopeFile->getType(currentLine, errors);
		}

		if (!foundType)
		{
			errors->error(ErrorCode::parseUnknownSymbol, value,
				"Unknown class: '" + className.string + "'");
			return ExpressionResult();
		}

		auto compiled = foundType->compileValue(className, currentLine, errors, scope, hintType);

		if (compiled.type == nullptr)
		{
			return ExpressionResult();
		}

		compiled.code.addBuffer(compiled.type->compileEndMove(scope));
		return compiled;
	}

	if (value == "await")
	{
		if (!scope->scopeFunction->isAsync)
		{
			errors->error(ErrorCode::parseInvalidAwait, value, "Await can only be used in async functions");
			return ExpressionResult();
		}

		auto expr = pushExpression(currentLine, errors, setExpression, hintType, scope);

		auto taskType = dynamic_cast<TaskType*>(expr.type);

		if (taskType)
		{
			return taskType->compileAwait(expr, scope->taskVariable->readExpression(scope), scope);
		}
		errors->error(ErrorCode::parseInvalidType, value, "Cannot await non task type: " + Type::toString(expr.type));
		return ExpressionResult();
	}

	// if it's a variable, it's a variable (shocking)
	auto foundVariable = scope->variables.find(value);

	if (foundVariable != scope->variables.end())
	{
		ExpressionResult result = foundVariable->second.readExpression(scope);
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
#ifdef WITH_LANGUAGE_SERVICE
		if (scope->context->service)
		{
			scope->context->service->files[scope->scopeFile->name].variables.push_back(
				ScannedVariable(&foundVariable->second, value));
		}
#endif
		return result;
	}

	auto constant = scope->scopeFile->getConstant(value, errors);

	if (constant.valid)
	{
#ifdef WITH_LANGUAGE_SERVICE
		if (scope->context->service)
		{
			auto var = ScannedVariable();

			var.kind = ScannedVariable::Kind::constant;
			var.at = value;
			var.name = value.string;
			var.typeId = constant.type->id;
			var.type = Type::toString(constant.type);

			scope->context->service->files[scope->scopeFile->name]
				.variables.push_back(var);
		}
#endif

		return constant;
	}

	auto prevPos = currentLine.savePosition();
	Function* function = scope->scopeFile->getMethod(value, currentLine, errors);
	if (function)
	{
		// Only compile if it's a function call
		if (currentLine.peek() == "(")
		{
			auto args = currentLine.getInBraces(errors);
			auto& argsEnd = currentLine.previous();

			TokenLine argsLine;
			argsLine.lineTokens = &args;

			GenericParseData generic = getGenericFunctionData(function, currentLine, errors, scope);

			ExpressionResult callCode = parseFunctionArguments(value, generic.args,
				argsLine, errors, true, scope);

#ifdef WITH_LANGUAGE_SERVICE
			if (scope->context->service)
			{
				scope->context->service->files[scope->scopeFile->name]
					.functions.push_back(ScannedFunction(function, value,
						ScannedFunction::Kind::functionCall, argsEnd.position));
			}
#endif

			callCode.code.addBuffer(generic.code);
			auto fn = function->compileCall();
			fn.type = generic.returnType;
			if (fn.type)
			{
				fn.code.addBuffer(fn.type->compileEndMove(scope));
			}

			callCode.code.addBuffer(fn.code);
			callCode.discardable = fn.discardable;
			callCode.type = fn.type;
			callCode.valid = fn.valid;

			return callCode;
		}

		currentLine.loadPosition(prevPos);
	}

	// Try to convert it into each default type.
	for (auto& i : scope->context->defaultTypes)
	{
		size_t pos = currentLine.savePosition();
		auto compiled = i->compileValue(value, currentLine, errors, scope, hintType);
		if (compiled.valid)
		{
			return compiled;
		}
		currentLine.loadPosition(pos);
	}

	if (scope->inClass)
	{
		currentLine.loadPosition(initialPosition);
		auto result = pushClassValue(currentLine, errors, setExpression, scope);

		if (result.valid)
			return result;
	}

	errors->error(ErrorCode::parseUnknownExpressionType, value, "Unexpected '" + value.string + "'");
	return ExpressionResult();
}

ExpressionResult ds::Expression::pushClassValue(TokenLine& currentLine,
	ErrorContext* errors, bool setExpression, ParsedScope* scope)
{
	return scope->inClass->thisType->compileMember(scope->thisVariable->readExpression(scope), currentLine,
		errors, setExpression, scope);
}

ExpressionResult ds::Expression::parseFunctionArguments(Token functionName, std::vector<FunctionArgument> arguments,
	TokenLine& currentLine, ErrorContext* errors, bool hasToMatch, ParsedScope* scope)
{
	ExpressionResult callCode;
	size_t argIndex = 0;

	while (!currentLine.empty())
	{
		auto& exprToken = currentLine.peek();

		Type* hintType = arguments.size() > argIndex ? arguments[argIndex].type : nullptr;

		auto expr = pushExpression(currentLine, errors, false, hintType, scope);

		if (arguments.size() <= argIndex)
		{
			if (expr.valid && hasToMatch)
			{
				errors->error(ErrorCode::parseUnexpectedToken, exprToken,
					"Unexpected argument of type " + Type::toString(expr.type) + " for function '" +
						functionName.string + "'. Only " + std::to_string(arguments.size()) +
						" argument(s) expected.");
			}
			return ExpressionResult();
		}

		auto& currentArg = arguments[argIndex++];
		expr.compileToType(exprToken, currentArg.type, scope, hasToMatch ? errors : nullptr);

		if (!expr.type || !expr.valid)
		{
			return ExpressionResult();
		}

		callCode.code.addBuffer(expr.code);
		callCode.code.addBuffer(expr.type->compileMove(scope));

		if (currentLine.empty())
			break;
		else
			currentLine.expect(",", errors);
	}

	if (argIndex < arguments.size())
	{
		if (hasToMatch)
		{
			errors->error(ErrorCode::parseUnexpectedToken,
				currentLine.lineTokens->size() ? currentLine.previous() : functionName,
				"Wrong number of arguments for function '" +
					functionName.string + "'. Got " + std::to_string(argIndex) + ", but " + std::to_string(arguments.size()) +
					" argument(s) were expected.");
		}
		return ExpressionResult();
	}

	callCode.valid = true;
	return callCode;
}