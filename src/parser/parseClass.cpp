#include <parser/parseClass.hpp>
#include <parser/parser.hpp>
#include <parser/parseScope.hpp>
using namespace lang;

bool lang::ParsedClass::scanLine(ErrorContext* errors, ParsedFile* file)
{
	TokenLine currentLine = classStream.next(errors);

	if (currentLine.empty())
		return false;

	auto type = file->getType(currentLine);

	// Class member variable
	if (type)
	{
		auto name = currentLine.get();

		auto equals = currentLine.get();

		auto value = currentLine.getUntil("", errors);

		this->members.insert({ name,
			ParsedClassMember{
				.type = type,
				.name = name,
				.value = value,
			} });
		return true;
	}

	currentLine.position = 0;

	bool isVirutal = false;
	bool isOverride = false;
	std::vector<Token> modifiers;

	while (true)
	{
		Token nextModifier = currentLine.peek();

		if (nextModifier == "virtual" || nextModifier == "override")
		{
			isVirutal = true;
			modifiers.push_back(currentLine.get());
			continue;
		}

		break;
	}

	auto first = currentLine.get();

	if (first == "fn")
	{
		auto* fn = new ParsedFunction();
		fn->functionModule = file->fileModule;
		fn->isVirtual = isVirutal;
		fn->scanDeclaration(currentLine, classStream, file, errors);
		fn->inClass = this;

		if (fn->name == "new")
		{
			fn->returnType = thisType;
		}
		else if (fn->name == "delete")
		{
			this->usedDestructor = fn;
		}

		this->methods.push_back(fn);
		return true;
	}

	errors->error(ErrorCode::parseUnexpectedToken,
		first, "Unexpected '" + first.string + "' in class definition");

	return true;
}

void lang::ParsedClass::compileDestructor(ParseContext* context, ErrorContext* errors, ParsedFile* file)
{
	ParsedScope destructorScope;
	destructorScope.scopeFile = file;
	destructorScope.context = context;
	destructorScope.code = &this->baseDestructor.code;
	destructorScope.setClass(this, false);

	auto cleanupCode = BytecodeBuffer();

	for (auto& i : this->members)
	{
		auto unrefCode = i.second.type->compileUnref();

		if (unrefCode.instructions.size())
		{
			cleanupCode.addBuffer(destructorScope.thisVariable->readValue(&destructorScope));
			cleanupCode.addBuffer(i.second.readValue());
			cleanupCode.addBuffer(unrefCode);
		}
	}

	if (cleanupCode.instructions.size())
	{
		baseDestructor.code.addBuffer(cleanupCode);

		destructorScope.compileScopeExit(true);
		baseDestructor.code.addOperation(BytecodeOp::ret);

		auto& destructorBytecode = context->compiler.functions[baseDestructor.getFullName()];
		destructorBytecode.instructions = baseDestructor.code.instructions;
	}
	else
	{
		baseDestructor.code.instructions.clear();
		if (&baseDestructor == this->usedDestructor)
		{
			this->usedDestructor = nullptr;
			thisType->destructor = this->usedDestructor;
		}
	}
}

ExpressionResult lang::ClassLifetimeFunction::compileCall()
{
	ExpressionResult result;
	result.code.add(new BytecodeCallFunction(getFullName()));
	result.type = parent->thisType;
	result.valid = true;
	return result;
}

std::vector<FunctionArgument> lang::ClassLifetimeFunction::getArguments()
{
	return {};
}

std::string lang::ClassLifetimeFunction::getFullName() const
{
	return this->parent->classModule->name + "::" + this->parent->name.string + (this->isConstructor ? ".new.base" : ".delete.base");
}

bool lang::ClassLifetimeFunction::discardable() const
{
	return true;
}

lang::ParsedClass::~ParsedClass()
{
}

void lang::ParsedClass::registerType(ParseContext* context, ParsedFile* file)
{
	thisType = new ClassType();
	thisType->from = name;
	thisType->name = name.string;
	file->fileModule->moduleTypes.insert({ name.string, thisType });
}

void lang::ParsedClass::scanClass(ParseContext* context, ParsedFile* file)
{
	std::vector<ClassType*> parents;
	for (auto& i : this->derivedFrom)
	{
		TokenLine line;
		line.lineTokens = &i;

		Type* type = file->fileModule->getType(line);

		if (!type)
		{
			context->errors.error(ErrorCode::parseExpectedName, line.previous(),
				"Expected a type name, got " + line.previous().string);
			continue;
		}

		auto classType = dynamic_cast<ClassType*>(type);

		if (!classType && type)
		{
			context->errors.error(ErrorCode::parseInvalidType, line.previous(),
				"Cannot inherit from '" + type->name + "', it isn't a class.");
			continue;
		}

		parents.push_back(classType);
	}

	std::vector<ClassMember> members;
	std::map<std::string, Function*> methods;

	this->classModule = file->fileModule;

	uint32_t position = 0;
	for (auto& m : this->members)
	{
		members.push_back(ClassMember{
			.name = m.second.name,
			.offset = position,
			.type = m.second.type,
		});
		m.second.offset = position,
		position += m.second.type->size;
	}

	std::vector<Function*> constructors;

	for (auto& i : this->methods)
	{
		if (i->name == "new")
		{
			constructors.push_back(i);
		}
		else
		{
			methods.insert({ i->name.string, i });
		}
	}

	constructor.parent = this;
	baseDestructor.parent = this;
	baseDestructor.isConstructor = false;
	thisType->members = members;
	thisType->methods = methods;
	thisType->classSize = position;
	thisType->baseConstructor = &this->constructor;
	thisType->destructor = this->usedDestructor;
	thisType->constructors = constructors;
	thisType->parents = parents;

	for (auto& m : this->methods)
	{
		m->resolveTypes(context, &context->errors);
	}
}

void lang::ParsedClass::compile(ParseContext* context, ErrorContext* errors, ParsedFile* file)
{
	ParsedScope constructorScope;
	constructorScope.scopeFile = file;
	constructorScope.context = context;
	constructorScope.code = &this->constructor.code;
	constructorScope.setClass(this, true);

	auto& code = constructorScope.code;

	for (auto& [name, member] : this->members)
	{
		if (member.value.empty())
			continue;

		TokenLine valueLine = TokenLine(&member.value);
		auto varExpr = constructorScope.pushExpression(valueLine, &context->errors, false);
		varExpr.compileToType(member.name, member.type, errors);
		code->addBuffer(varExpr.code);
		code->addBuffer(varExpr.type->compileMove(&constructorScope));
		code->addBuffer(constructorScope.thisVariable->readValue(&constructorScope));

		code->pushInt(member.offset);
		code->pushInt(member.type->size);

		code->addOperation(BytecodeOp::setClassMember);
	}

	compileDestructor(context, errors, file);

	for (auto& i : this->methods)
	{
		if (i->name == "new")
		{
			// do not pop the return value of the base constructor, so we still have
			// a pointer to this
			i->functionCode.add(new BytecodeCallFunction(this->constructor.getFullName()));
		}
		i->compile(context, file, errors);

		auto& bytecodeFunction = context->compiler.functions[i->getFullName()];
		bytecodeFunction.instructions = i->functionCode.instructions;
	}

	// Return a reference to this.
	code->addBuffer(constructorScope.thisVariable->readValue(&constructorScope));
	constructorScope.compileScopeExit(true);
	code->addOperation(BytecodeOp::ret);


	auto& constructorBytecode = context->compiler.functions[constructor.getFullName()];
	constructorBytecode.instructions = constructor.code.instructions;

	this->thisType->vTableOffset = context->virtualTable.size();
	size_t it = 1;
	context->virtualTable.push_back(usedDestructor);

	for (auto& i : this->methods)
	{
		if (i->isVirtual)
		{
			i->vTableOffset = it++;
			context->virtualTable.push_back(i);
		}
	}
}

void lang::ParsedClass::scan(ErrorContext* errors, ParsedFile* file)
{
	this->usedDestructor = &baseDestructor;
	while (scanLine(errors, file))
	{
	}
}

BytecodeBuffer lang::ParsedClassMember::readValue() const
{
	BytecodeBuffer out;
	out.pushInt(offset);
	out.pushInt(type->size);

	out.addOperation(BytecodeOp::classMember);
	return out;
}

BytecodeBuffer lang::ParsedClassMember::writeValue() const
{
	BytecodeBuffer out;
	out.pushInt(offset);
	out.pushInt(type->size);

	out.addOperation(BytecodeOp::setClassMember);
	return out;
}
