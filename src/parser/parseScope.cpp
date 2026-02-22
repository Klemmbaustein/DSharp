#include <ds/parser/parseScope.hpp>
#include <ds/parser/bytecode/compileBytecodeVariables.hpp>
#include <ds/parser/bytecode/compileBytecodeUnwind.hpp>
#include <ds/parser/types/arrayType.hpp>
#include <ds/parser/types/lambdaType.hpp>
#include <ds/service/languageService.hpp>
#include <ds/parser/parseExpression.hpp>
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

void ds::ParsedScope::parseSubScope(ParsedFile* file, ErrorContext* errors,
	BytecodeJumpLabel* breakTarget, BytecodeJumpLabel* continueTarget,
	size_t breakContinueDepth, ScopeOptions options)
{
	// Create a sub scope that optionally takes in the ScopeOptions
	ParsedScope conditionScope;
	conditionScope.scopeFunction = options.scopeFunction ? options.scopeFunction : this->scopeFunction;
	conditionScope.code = options.targetBuffer ? options.targetBuffer : this->code;
	conditionScope.tempCounter = this->tempCounter;
	conditionScope.breakTarget = breakTarget;
	conditionScope.inClass = inClass;
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

#ifdef WITH_LANGUAGE_SERVICE
void ds::ParsedScope::serializeScope()
{
	if (context->service)
	{
		auto s = ScannedScope(tokenStream->first, tokenStream->last);

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

BytecodeBuffer ds::ParsedScope::compileScopeExit(size_t toDepth, bool isEnd, bool dereferenceAll)
{
	BytecodeBuffer code;
	uint32_t size = 0;

	// Find the variables to erase because erasing variables while iterating them at the same time is a bad idea
	std::vector<Token> toErase;

	for (auto& i : variables)
	{
		bool isLambdaOwned = !this->isLambda || i.second.ownedBy == this;
		if (i.second.depth < toDepth || !isLambdaOwned)
		{
			continue;
		}

		bool isDestructor = scopeFunction && scopeFunction->name == "delete";
		bool isConstructor = scopeFunction && scopeFunction->name == "new";
		bool varIsThis = &i.second == thisVariable;
		bool shouldUnrefThis = ((!isDestructor && !returnThis) || isConstructor);

		if (dereferenceAll && (!varIsThis || shouldUnrefThis) && this->taskVariable != &i.second)
		{
			auto unrefCode = i.second.type->compileUnref();
			if (unrefCode.instructions.size())
			{
				code.addNew<BytecodeUnwindClass>(i.second.variableInstruction);
				code.addBuffer(i.second.readValue(this));
				code.addBuffer(unrefCode);
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

	// Also call the base (compiler generated) destructor if this is a destructor function
	if (inClass && scopeFunction && scopeFunction->name == "delete")
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
		code.addNew<BytecodePopVariable>(size, !isEnd);

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

	if (code->instructions.size() && (*code->instructions.rbegin())->operation == BytecodeOp::ret)
	{
		return;
	}

	serializeScope();

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

		parseSubScope(file, errors, endLabel.get(), beginLabel.get(), this->depth + 1);
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
			if (!valueExpr.valid)
			{
				errors->error(ErrorCode::parseInvalidType, line.previous(), "Failed to compile operator");
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
	// if [expr] [
	//     [scope]
	// }
	// else ...
	auto conditionLine = line.getUntil("{", errors);

	TokenLine conditionTokens;
	conditionTokens.lineTokens = &conditionLine;

	auto condition = Expression::pushExpression(conditionTokens, errors, false, nullptr, this);
	conditionTokens.expectEndOfLine(errors);
	condition.compileToType(conditionTokens.previous(), file->context->registry->getEntry<BoolType>(), this, errors);
	this->code->addBuffer(condition.code);

	// The end of the if-statement scope, might also be the start of an else statement if there is one
	auto endLabel = std::make_shared<BytecodeJumpLabel>("endif");

	this->code->addNew<BytecodeJump>(BytecodeOp::jumpIfNot, endLabel.get());

	parseSubScope(file, errors, this->breakTarget, this->continueTarget, breakContinueDepth);

	auto nextLine = this->tokenStream->peek(errors);

	if (nextLine.peek() == "else")
	{
		this->tokenStream->next(errors);

		nextLine.get();

		// Jump to the end of the else statement if the initial condition was true
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

	auto arrayType = dynamic_cast<ArrayType*>(arrayExpression.type);

	if (!arrayType)
	{
		errors->error(ErrorCode::parseUnexpectedToken, var->equals,
			"For loop expression needs to be an array. Got: " + Type::toString(arrayExpression.type));
		return;
	}

	// Increment the scope depth so the loop has it's own scope to store the iterated array
	this->depth++;

	this->code->addBuffer(arrayExpression.code);
	pushVariableValue(arrayExpression.type, true);

	// Save the array to a temporary variable so the array lives for the entire loop
	// For example, with for int i = [1, 2]
	// The array would need to be reloaded for each iteration unless it's a variable
	auto& iterated = addVariable(Token(".for_iterated" + std::to_string(tempCounter++)), arrayType, errors);
	iterated.isInternal = true;

	// The iterator is an int that stores the current index of the array
	auto iterType = context->registry->getEntry<IntType>();
	this->code->addBuffer(iterType->defaultValue().code);
	pushVariableValue(iterType, true);
	auto& iterator = addVariable(Token(".for_iterator" + std::to_string(tempCounter++)), iterType, errors);
	iterator.isInternal = true;

	// The label for restarting the loop
	auto beginLabel = std::make_shared<BytecodeJumpLabel>("for_begin");
	// The label to jump to when continuing the loop with the continue statement
	auto continueLabel = std::make_shared<BytecodeJumpLabel>("for_continue");
	this->code->add(beginLabel);

	// The end of the loop, when
	auto endLabel = std::make_shared<BytecodeJumpLabel>("for_end");
	this->code->addBuffer(arrayType->getLength(iterated.readValue(this), context).code);
	this->code->addBuffer(iterator.readValue(this));
	this->code->addOperation(BytecodeOp::greaterInt);

	// Go to the endif the array length is not greater than the iterator
	this->code->addNew<BytecodeJump>(BytecodeOp::jumpIfNot, endLabel.get());

	// Increment the scope for the for loop variable, which technically isn't part of the for loop's scope
	this->depth++;

	auto index = arrayType->compileIndex(iterated.readExpression(this), iterator.readExpression(this),
		errors, false, this);

	if (!var->type)
	{
		var->type = index.type;
	}
	index.compileToType(var->equals, var->type, this, errors);

	var->assignedValue = index;

	var->create(this, errors);

	parseSubScope(file, errors, endLabel.get(), continueLabel.get(), this->depth + 1);

	this->code->add(continueLabel);
	code->addBuffer(compileScopeExit(this->depth, true));
	// Increment the iterator
	this->code->addBuffer(iterator.readValue(this));
	this->code->pushInt(1);
	this->code->addOperation(BytecodeOp::addInt);
	this->code->addBuffer(iterator.writeValue());

	// jump to the start and repeat the loop again, it will jump back to the end label if the loop should end
	this->code->addNew<BytecodeJump>(BytecodeOp::jump, beginLabel.get());
	this->code->add(endLabel);

	this->depth -= 2;
	code->addBuffer(compileScopeExit(this->depth + 1, true));
}

BytecodeBuffer ds::ScopeVariable::readValue(ParsedScope* with) const
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

BytecodeBuffer ds::ScopeVariable::writeValue() const
{
	BytecodeBuffer result;
	result.addNew<BytecodeStoreVariable>(this->variableInstruction);
	return result;
}

ExpressionResult ds::ScopeVariable::readExpression(ParsedScope* with) const
{
	ExpressionResult result;
	result.valid = true;
	result.type = this->type;
	result.code = readValue(with);
	return result;
}
