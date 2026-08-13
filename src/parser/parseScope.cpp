#include <ds/parser/parseScope.hpp>
#include <ds/parser/bytecode/compileBytecodeVariables.hpp>
#include <ds/parser/bytecode/compileBytecodeUnwind.hpp>
#include <ds/parser/types/arrayType.hpp>
#include <ds/parser/types/lambdaType.hpp>
#include <ds/service/languageService.hpp>
#include <ds/parser/parseExpression.hpp>
#include <ds/parser/bytecode/constantEvaluate.hpp>
#include <ds/parser/types/iteratorType.hpp>
using namespace ds;

void ds::ParsedScope::returnCompletedTask(TaskType* taskType)
{
	this->code->addBuffer(taskVariable->readValue(this));
	auto completed = taskType->compileCompleteTask();
	this->code->addBuffer(completed.code);
}

std::optional<VariableInfo> ds::ParsedScope::parseVariableDefinition(TokenLine& line, ParsedFile* file,
	ErrorContext* errors, bool matchTypes)
{
	VariableInfo info;

	auto& first = line.peek();

	info.type = file->getType(line, errors);

	// If there is a type, it's probably a variable declaration.
	if (!info.type && first != "var" && first != "const")
	{
		return {};
	}

	info.isVar = first == "var";
	info.isConst = first == "const";

	if (info.isVar || info.isConst)
	{
		// var/const
		line.get();
	}

	info.name = line.get();
	info.name.checkIsName(errors);

	BinaryBuffer args;

	info.equals = line.get();
	if (info.equals == "=")
	{
		info.assignedValue = Expression::pushExpression(line, errors, false, info.type, this);

		if (!info.assignedValue.valid)
		{
			if (info.isVar || info.isConst)
			{
				return VariableInfo{
					.isError = true
				};
			}
		}
		info.isError = true;

		if (info.isVar || info.isConst)
		{
			if (!info.assignedValue.type)
			{
				errors->error(ErrorCode::parseInvalidType, info.name,
					"Value of variable declaration does not have a type.");
			}
			if (matchTypes)
			{
				info.type = info.assignedValue.type;
			}
		}
		else if (matchTypes)
		{
			info.assignedValue.compileToType(info.equals, info.type, this, errors);
		}
	}
	else if (!info.equals.empty())
	{
		errors->error(ErrorCode::parseVarMustHaveInitializer, info.equals,
			"Expected a '=', got: '" + info.equals.string + "'");
		return VariableInfo{
			.isError = true
		};
	}
	else if (info.isVar || info.isConst)
	{
		errors->error(ErrorCode::parseVarMustHaveInitializer, info.name,
			"A variable declared with 'var' or 'const' must have an initializer.");
		return VariableInfo{
			.isError = true
		};
	}
	else
	{
		if (!info.type->hasDefaultValue)
		{
			errors->error(ErrorCode::parseVarMustHaveInitializer, info.name,
				"The variable '" + info.name.string + "' must have an initializer because the type '" +
					Type::toString(info.type) + "' does not have a default value.");
			return VariableInfo{
				.isError = true
			};
		}

		info.assignedValue = info.type->defaultValue();
	}

	return info;
}

void ds::VariableInfo::create(ParsedScope* in, ErrorContext* errors) const
{
	if (!type)
	{
		return;
	}

	in->code->addBuffer(this->assignedValue.code);
	in->pushVariableValue(type, true);

	auto& newVariable = in->addVariable(this->name, type, errors);
	newVariable.readOnly = isConst;
#ifdef WITH_LANGUAGE_SERVICE
	if (in->context->service)
	{
		in->context->service->files[in->scopeFile->name].variables.push_back(
			ScannedVariable(&newVariable, this->name));
	}
#endif
}

void ds::ParsedScope::parseSubScope(Token beginToken, ParsedFile* file, ErrorContext* errors,
	BytecodeJumpLabel* breakTarget, BytecodeJumpLabel* continueTarget,
	size_t breakContinueDepth, ScopeOptions options)
{
	// Create a sub scope that optionally takes in the ScopeOptions
	ParsedScope conditionScope;
	conditionScope.beginToken = beginToken;
	conditionScope.scopeFunction = options.scopeFunction ? options.scopeFunction : this->scopeFunction;
	conditionScope.code = options.targetBuffer ? options.targetBuffer : this->code;
	conditionScope.tempCounter = this->tempCounter;
	conditionScope.breakTarget = breakTarget;
	conditionScope.inClass = inClass;
	conditionScope.returnThis = !options.isLambda && returnThis;
	conditionScope.continueTarget = continueTarget;
	conditionScope.taskVariable = options.isLambda ? nullptr : this->taskVariable;
	conditionScope.breakContinueDepth = breakContinueDepth;
	conditionScope.depth = this->depth + 1;
	conditionScope.isLambda = options.isLambda;
	conditionScope.functionDepth = options.isLambda ? conditionScope.depth : functionDepth;

	if (conditionScope.isLambda && conditionScope.scopeFunction && conditionScope.scopeFunction->isAsync)
	{
		conditionScope.addTask(reinterpret_cast<TaskType*>(conditionScope.scopeFunction->returnType));
	}
	if (conditionScope.isLambda)
	{
		conditionScope.lambdaRootScope = &conditionScope;
	}
	else
	{
		conditionScope.lambdaRootScope = this->lambdaRootScope;
	}
	conditionScope.lambdaVariable = this->lambdaVariable;

	for (auto& i : this->variables)
	{
		if (i.second.ownedBy && !(i.second.isInternal && options.isLambda))
		{
			auto [inserted, _] = conditionScope.variables.insert(i);
			if (&i.second == this->taskVariable)
			{
				conditionScope.taskVariable = &inserted->second;
			}
			if (&i.second == this->thisVariable)
			{
				conditionScope.thisVariable = &inserted->second;
			}
		}
	}

	TokenStream localStream;
	if (!options.scopeTokens)
	{
		this->tokenStream->getScope(localStream, errors);
		options.scopeTokens = &localStream;
	}

	conditionScope.variableStackPosition = this->variableStackPosition;
	conditionScope.tokenStream = options.scopeTokens;

	conditionScope.compile(this->context, file, errors);
}

#ifdef WITH_LANGUAGE_SERVICE
void ds::ParsedScope::serializeScope()
{
	if (context->service)
	{
		auto s = ScannedScope(beginToken.position, tokenStream->last);

		for (auto& i : this->variables)
		{
			if (!i.second.isInternal)
			{
				s.localVariables.push_back(ScannedScopeVariable{
					.name = i.second.name.string,
					.type = i.second.type->id,
					.isThis = this->inClass && i.second.name == "this" });
			}
		}

		context->service->files[scopeFile->name].scopes.push_back(s);
	}
}
#endif

BytecodeBuffer ds::ParsedScope::addTemporaryVariable(Type* type)
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
		.variableInstruction = instruction.get(),
		.ownedBy = this,
		.depth = this->depth,
		.type = type,
		.isInternal = true,
	} }).first->second;
	// clang-format on

	this->variableStackPosition += type->size;

	return buffer;
}

void ds::ParsedScope::pushVariableValue(Type* type, bool copy)
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

ScopeVariable& ds::ParsedScope::addVariable(Token name, Type* type, ErrorContext* errors)
{
	auto instruction = std::make_shared<BytecodePushVariable>(name.string, type);
	code->add(instruction);

	auto found = this->variables.find(name);

	if (found != this->variables.end() && errors)
	{
		errors->error(ErrorCode::parseVariableRedefinition, name, "Variable redefinition of '" + name.string + "'");
		errors->error(ErrorCode::parseVariableRedefinition, found->first, "'" + name.string + "' first defined here.");
		return this->variables[name];
	}

	auto result = this->variables.insert({ name,
		ScopeVariable{
			.name = name,
			.variableInstruction = instruction.get(),
			.ownedBy = this,
			.depth = this->depth,
			.type = type,
		} });

	this->variableStackPosition += type->size;

	return result.first->second;
}

BytecodeBuffer ds::ParsedScope::compileScopeExit(size_t toDepth, bool isEnd, bool dereferenceAll,
	bool unreachable)
{
	BytecodeBuffer code;
	uint32_t size = 0;

	// Find the variables to erase because erasing variables while iterating them at the same time is a bad idea
	std::vector<Token> toErase;

	for (auto& i : variables)
	{
		if (i.second.variableInstruction->isInvalid)
		{
			continue;
		}
		bool isLambdaOwned = !this->lambdaRootScope || i.second.ownedBy == this;
		if (i.second.depth < toDepth || !isLambdaOwned)
		{
			continue;
		}

		bool isDestructor = scopeFunction && scopeFunction->name == "delete";
		bool isConstructor = scopeFunction && scopeFunction->name == "new";
		bool varIsThis = &i.second == thisVariable;
		bool varIsThisTask = &i.second == taskVariable;
		bool shouldUnrefThis = ((!isDestructor && !returnThis) || isConstructor);

		if (dereferenceAll && !unreachable && !varIsThisTask && (!varIsThis || shouldUnrefThis))
		{
			auto unrefCode = i.second.type->compileUnref();
			if (unrefCode.instructions.size())
			{
				if (isEnd)
					code.addNew<BytecodeUnwindClass>(i.second.variableInstruction);
				code.addBuffer(i.second.readValue(this));
				code.addBuffer(unrefCode);
			}

			if (isEnd && &i.second != thisVariable)
			{
				toErase.push_back(i.first);
			}
		}
		else if (isEnd && dereferenceAll && !unreachable && varIsThisTask)
		{
			code.addNew<BytecodeUnwindClass>(i.second.variableInstruction);
		}

		size += i.second.type->size;
	}

	for (auto& i : toErase)
	{
		variables.erase(i);
	}

	// Also call the base (compiler generated) destructor if this is a destructor function
	if (inClass && toDepth == 0 && scopeFunction && scopeFunction->name == "delete")
	{
		code.addBuffer(thisVariable->readValue(this));

		if (inClass->baseDestructor.code && inClass->baseDestructor.code->instructions.size())
		{
			code.addBuffer(inClass->baseDestructor.compileCall().code);
		}
		else
		{
			code.addOperation(BytecodeOp::unrefClass);
		}
	}

	if (size)
	{
		code.addNew<BytecodePopVariable>(size, !isEnd, unreachable);

		this->variableStackPosition -= size;
	}

	return code;
}

void ds::ParsedScope::setClass(ParsedClass* inClass, bool copy)
{
	this->inClass = inClass;

	if (copy)
	{
		this->code->addBuffer(inClass->thisType->compileMove(this));
	}
	pushVariableValue(inClass->thisType, false);
	this->thisVariable = &addVariable(Token("this"), inClass->thisType, nullptr);
	this->thisVariable->readOnly = true;
	this->thisVariable->isThis = true;
}

void ds::ParsedScope::addTask(TaskType* taskType)
{
	this->code->addBuffer(taskType->compileTask().code);
	this->code->addBuffer(taskType->compileMove(this));
	pushVariableValue(taskType, false);
	this->taskVariable = &addVariable(Token(".task" + std::to_string(tempCounter++)), taskType, nullptr);
	this->taskVariable->isInternal = true;
}

void ds::ParsedScope::compile(ParseContext* context, ParsedFile* file, ErrorContext* errors)
{
	this->context = context;
	this->scopeFile = file;

	if (isLambda)
	{
		pushVariableValue(LambdaType::getInstance(), true);
		lambdaVariable = &addVariable(Token(".lambda"), LambdaType::getInstance(), nullptr);
		lambdaVariable->isInternal = true;
		this->scopeFunction->addArguments(*this, errors);
	}

	while (true)
	{
		auto nextLine = this->tokenStream->next(&context->errors);

		if (nextLine.empty())
			break;

		compileLine(nextLine, file, errors);
	}

#ifdef WITH_LANGUAGE_SERVICE
	serializeScope();
#endif

	if (code->instructions.size() && (*code->instructions.rbegin())->operation == BytecodeOp::ret)
	{
		// Still pop the variable stack even if this scope ends with a return.
		code->addBuffer(compileScopeExit(this->depth, true, true, true));
		return;
	}

	code->addBuffer(compileScopeExit(this->depth, true));
	if (compileReturn || this->isLambda)
	{
		Type* returnType = this->scopeFunction ? this->scopeFunction->returnType : nullptr;
		if (returnThis)
		{
			code->addBuffer(thisVariable->readValue(this));
			code->addOperation(BytecodeOp::ret);
			return;
		}
		else if (this->taskVariable)
		{
			auto taskType = static_cast<TaskType*>(taskVariable->type);
			returnType = taskType->baseType;
			if (!returnType)
			{
				this->returnCompletedTask(taskType);
				code->addOperation(BytecodeOp::ret);
				return;
			}
		}
		if (returnType)
		{
			// Function not ending in a return statement should abort if it ever reaches the end.
			// You could write code in which the normal end of the function is unreachable and there's no nice way to detect this,
			// so this seems like the best solution.
			code->addOperation(BytecodeOp::noReturn);
		}
		else
		{
			code->addOperation(BytecodeOp::ret);
		}
	}
}

void ds::ParsedScope::compileLine(TokenLine line, ParsedFile* file, ErrorContext* errors)
{
	auto& first = line.get();

	if (first == "return" && scopeFunction)
	{
		auto type = scopeFunction->returnType;
		auto taskType = taskVariable ? static_cast<TaskType*>(taskVariable->type) : nullptr;
		if (taskType)
		{
			type = taskType->baseType;
		}
		if (type)
		{
			if (line.empty())
			{
				errors->error(ErrorCode::parseUnknownExpressionType, first,
					"Return statement doesn't return a value, but it must return '" + Type::toString(type) + "'");
				return;
			}

			auto expr = Expression::pushExpression(line, errors, false, type, this);

			if (!expr.valid)
			{
				return;
			}
			expr.compileToType(first, type, this, errors);

			this->code->addBuffer(expr.code);
			if (expr.type)
			{
				this->code->addBuffer(expr.type->compileMove(this));
			}
		}

		if (taskType)
		{
			returnCompletedTask(taskType);
		}
		else if (returnThis)
		{
			code->addBuffer(thisVariable->readValue(this));
		}

		code->addBuffer(compileScopeExit(0, false));
		this->code->addOperation(BytecodeOp::ret);
		line.expectEndOfLine(errors);
		return;
	}
	if (first == "_")
	{
		if (line.expect("=", errors))
		{
			return;
		}

		auto expr = Expression::pushExpression(line, errors, true, nullptr, this);
		this->code->addBuffer(expr.code);
		if (expr.type)
		{
			BinaryBuffer args;
			args.addValue<uint32_t>(expr.type->size);
			this->code->addOperation(BytecodeOp::pop, args);
		}
		else if (expr.valid)
		{
			errors->error(ErrorCode::parseInvalidType, first, "Cannot discard expression that doesn't return a value.");
		}

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
			code->addBuffer(this->compileScopeExit(breakContinueDepth, false));
			this->code->addNew<BytecodeJump>(BytecodeOp::jump, this->breakTarget);
			return;
		}
	}

	if (first == "continue")
	{
		if (this->continueTarget)
		{
			code->addBuffer(this->compileScopeExit(breakContinueDepth, false));
			this->code->addNew<BytecodeJump>(BytecodeOp::jump, this->continueTarget);
			return;
		}
	}

	if (first == "while")
	{
		auto conditionLine = line.getUntil("{", errors);

		TokenLine conditionTokens;
		conditionTokens.lineTokens = &conditionLine;

		auto boolType = context->registry->getEntry<BoolType>();

		auto condition = Expression::pushExpression(conditionTokens, errors, false,
			boolType, this);
		conditionTokens.expectEndOfLine(errors);
		condition.compileToType(first, boolType, this, errors);

		auto beginLabel = std::make_shared<BytecodeJumpLabel>("while_begin");
		this->code->add(beginLabel);
		this->code->addBuffer(condition.code);

		auto endLabel = std::make_shared<BytecodeJumpLabel>("while_end");

		this->code->addNew<BytecodeJump>(BytecodeOp::jumpIfNot, endLabel.get());

		parseSubScope(line.previous(), file, errors, endLabel.get(), beginLabel.get(), this->depth + 1);
		this->code->addNew<BytecodeJump>(BytecodeOp::jump, beginLabel.get());

		this->code->add(endLabel);

		return;
	}

	line.position = 0;

	auto variable = parseVariableDefinition(line, file, errors);

	if (variable)
	{
		variable->create(this, errors);
		line.expectEndOfLine(errors);
		return;
	}

	line.position = 0;
	auto expr = Expression::pushExpression(line, errors, true, nullptr, this);

	auto compoundOperator = stringToCompoundOperator(line.peek().string);
	if ((line.peek() == "=" || compoundOperator != CompoundOperator::unknown) && expr.valid)
	{
		auto& equals = line.get();

		if (!expr.setCode)
		{
			errors->error(ErrorCode::parseReadOnlyValue, line.lineTokens->at(0),
				"This value cannot be written to.");
			return;
		}

		auto valueExpr = Expression::pushExpression(line, errors, false, expr.type, this);
		if (!valueExpr.type || !valueExpr.valid)
		{
			return;
		}
		valueExpr.compileToType(equals, expr.type, this, errors);
		if (!valueExpr.type)
		{
			return;
		}

		if (compoundOperator != CompoundOperator::unknown)
		{
			valueExpr = expr.type->compileOperator(
				Operator(compoundOperator), expr, valueExpr, this);
			if (!valueExpr.valid || !valueExpr.type)
			{
				errors->error(ErrorCode::parseInvalidType, line.previous(), "Failed to compile operator");
				return;
			}
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
			BinaryBuffer args;
			args.addValue<uint32_t>(expr.type->size);
			this->code->addOperation(BytecodeOp::pop, args);
			expr.discard(line.lineTokens->at(0), errors);
		}
		line.expectEndOfLine(errors);
	}
}

void ds::ParsedScope::compileIf(TokenLine line, ParsedFile* file, ErrorContext* errors)
{
	// if [const?] [expr] [
	//     [scope]
	// }
	// else ...

	// If the if statement is const, it is a compile time condition
	bool isConst = line.peek() == "const" && (line.get(), true);

	auto conditionLine = line.getUntil("{", errors);

	TokenLine conditionTokens;
	conditionTokens.lineTokens = &conditionLine;

	auto condition = Expression::pushExpression(conditionTokens, errors, false, nullptr, this);
	conditionTokens.expectEndOfLine(errors);
	condition.compileToType(conditionTokens.previous(), file->context->registry->getEntry<BoolType>(), this, errors);

	bool constValue = false;

	if (!isConst)
	{
		this->code->addBuffer(condition.code);
	}
	else if (condition.valid)
	{
		ConstantEvaluate evaluator;
		if (evaluator.run(condition.code))
		{
			constValue = evaluator.popValue<Bool>();
		}
		else
		{
			errors->error(ErrorCode::parseNotConst, conditionTokens.previous(),
				"'if const' expression is not a constant");
		}
	}

#ifdef WITH_LANGUAGE_SERVICE
	// Still parse the scope if running with a language service so we still get syntax highlighting
	const bool isService = file->context->service;
#else
	const bool isService = false;
#endif

	// The end of the if-statement scope, might also be the start of an else statement if there is one
	auto endLabel = isConst ? nullptr : std::make_shared<BytecodeJumpLabel>("endif");

	if (isConst)
	{
		if (constValue || isService)
		{
			parseSubScope(line.previous(), file, errors, this->breakTarget, this->continueTarget, breakContinueDepth);
		}
		else
		{
			// Ignore the next scope
			TokenStream stream;
			this->tokenStream->getScope(stream, errors);
		}
	}
	else
	{
		this->code->addNew<BytecodeJump>(BytecodeOp::jumpIfNot, endLabel.get());
		parseSubScope(line.previous(), file, errors, this->breakTarget, this->continueTarget, breakContinueDepth);
	}
	auto nextLine = this->tokenStream->peek(errors);

	if (nextLine.peek() == "else")
	{
		this->tokenStream->next(errors);

		nextLine.get();
		auto endElseLabel = isConst ? nullptr : std::make_shared<BytecodeJumpLabel>("endElseLabel");

		if (!isConst)
		{
			// Jump to the end of the else statement if the initial condition was true
			this->code->addNew<BytecodeJump>(BytecodeOp::jump, endElseLabel.get());
		}
		if (nextLine.peek() == "if")
		{
			nextLine.get();
			if (isConst)
			{
				if (!constValue || isService)
				{
					compileIf(nextLine, file, errors);
				}
				else
				{
					TokenStream stream;
					this->tokenStream->getScope(stream, errors);
				}
			}
			else
			{
				this->code->add(endLabel);
				compileIf(nextLine, file, errors);
				this->code->add(endElseLabel);
			}
		}
		else
		{
			if (!isConst)
			{
				this->code->add(endLabel);
				parseSubScope(line.previous(), file, errors, this->breakTarget, this->continueTarget, breakContinueDepth);
				this->code->add(endElseLabel);
			}
			else if (!constValue || isService)
			{
				parseSubScope(line.previous(), file, errors, this->breakTarget, this->continueTarget, breakContinueDepth);
			}
			else
			{
				TokenStream stream;
				this->tokenStream->getScope(stream, errors);
			}
		}
	}
	else if (endLabel)
	{
		this->code->add(endLabel);
	}
}

void ds::ParsedScope::compileFor(TokenLine line, ParsedFile* file, ErrorContext* errors)
{
	// for [var-def] {
	//     [scope]
	// }

	auto var = parseVariableDefinition(line, file, errors, false);

	if (!var)
	{
		errors->error(ErrorCode::parseUnexpectedToken, line.previous(),
			"Expected a variable definition");
		return;
	}

	auto& arrayExpression = var->assignedValue;
	if (!arrayExpression.valid)
	{
		errors->error(ErrorCode::parseUnexpectedToken, line.previous(),
			"For loop variable needs to have an array value assigned");
		return;
	}

	auto arrayType = arrayExpression.type->asClass();

	if (!arrayType)
	{
		errors->error(ErrorCode::parseUnexpectedToken, var->equals,
			"For loop expression needs to be a class. Got: " + Type::toString(arrayExpression.type));
		return;
	}

	// Increment the scope depth so the loop has it's own scope to store the iterated array
	this->depth++;

	auto getIteratorCall = arrayType->compileMethodDirect(Token("getIterator"), arrayExpression, errors, this);

	if (!getIteratorCall.valid)
	{
		errors->error(ErrorCode::parseInvalidType, var->equals,
			"For loop expression needs to implement getIterator");
		return;
	}

	this->code->addBuffer(getIteratorCall.code);
	auto iterType = dynamic_cast<IteratorType*>(getIteratorCall.type);
	pushVariableValue(iterType, true);
	auto& iterator = addVariable(Token(".for_iterator" + std::to_string(tempCounter++)), iterType, errors);
	iterator.isInternal = true;

	// The label for restarting the loop
	auto beginLabel = std::make_shared<BytecodeJumpLabel>("for_begin");
	// The label to jump to when continuing the loop with the continue statement
	auto continueLabel = std::make_shared<BytecodeJumpLabel>("for_continue");
	this->code->add(beginLabel);

	auto endLabel = std::make_shared<BytecodeJumpLabel>("for_end");
	// Move to the next iterated value.
	this->code->addBuffer(iterType->compileNext(iterator.readValue(this), errors, this));

	// Go to the end label if compileNext pushed false onto the stack (the iterator has ended)
	this->code->addNew<BytecodeJump>(BytecodeOp::jumpIfNot, endLabel.get());

	this->depth++;

	// Get the iterated value, move it to a variable and parse the inner scope
	auto index = iterType->compileGet(iterator.readValue(this), errors, this);

	if (!var->type)
	{
		var->type = index.type;
	}
	index.compileToType(var->equals, var->type, this, errors);

	var->assignedValue = index;

	var->create(this, errors);

	parseSubScope(line.previous(), file, errors, endLabel.get(), continueLabel.get(), this->depth + 1);

	this->code->add(continueLabel);
	code->addBuffer(compileScopeExit(this->depth, true));

	// jump to the start and repeat the loop again, it will jump back to the end label if the loop should end
	this->code->addNew<BytecodeJump>(BytecodeOp::jump, beginLabel.get());
	this->code->add(endLabel);

	this->depth -= 2;
	code->addBuffer(compileScopeExit(this->depth + 1, true));
}

BytecodeBuffer ds::ScopeVariable::readValue(ParsedScope* with) const
{
	if (this->variableInstruction->isInvalid)
	{
		with->context->errors.error(ErrorCode::internalError, Token(),
			"Internal error, temporary variable read after invalidated.");
	}

	BytecodeBuffer result;
	if (with->lambdaRootScope && ownedBy != with->lambdaRootScope && (ownedBy->depth < with->lambdaRootScope->depth))
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

BytecodeBuffer ds::ScopeVariable::writeValue() const
{
	BytecodeBuffer result;
	result.addNew<BytecodeStoreVariable>(this->variableInstruction);
	return result;
}

ExpressionResult ds::ScopeVariable::readExpression(ParsedScope* with) const
{
	if (this->variableInstruction->isInvalid)
	{
		with->context->errors.error(ErrorCode::internalError, Token(),
			"Internal error, temporary variable read after invalidated.");
	}
	ExpressionResult result;
	result.valid = true;
	result.type = this->type;
	result.code = readValue(with);
	return result;
}
