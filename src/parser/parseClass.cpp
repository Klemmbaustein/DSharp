#include <ds/parser/parseClass.hpp>
#include <ds/parser/parser.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/service/languageService.hpp>
#include <ds/parser/parseExpression.hpp>
using namespace ds;

bool ds::ParsedClass::scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors, ParsedFile* file)
{
	TokenLine currentLine = classStream.next(errors);

	if (currentLine.empty())
		return false;

	auto type = file->getType(currentLine, errors);

	// Class member variable
	if (type)
	{
		Token name = currentLine.get();
		Token equals = currentLine.get();

		auto value = currentLine.getUntil("", errors);

		auto [newMember, added] = this->members.insert({ name,
			ParsedClassMember{
				.type = type,
				.name = name,
				.value = value,
			} });

		newMember->second.attributes = currentAttributes;
		currentAttributes.clear();

		return true;
	}

	currentLine.position = 0;

	bool isVirtual = false;
	bool isOverride = false;
	bool isAsync = false;
	std::vector<Token> modifiers;

	auto next = currentLine.get();

	if (next == "[")
	{
		auto attribTokens = currentLine.getUntil("]", errors);

		if (attribTokens.empty())
		{
			errors->error(ErrorCode::parseUnexpectedToken, next, "Expected an attribute name after '['");
			return true;
		}

		currentAttributes.push_back(AttribInfo(attribTokens));
		return true;
	}

	while (true)
	{
		if (next == "virtual" || next == "override")
		{
			isVirtual = true;
			if (next == "override")
			{
				isOverride = true;
			}
			modifiers.push_back(next);
			next = currentLine.get();
			continue;
		}
		if (next == "async")
		{
			isAsync = true;
			next = currentLine.get();
			continue;
		}

		break;
	}

	if (next == "fn")
	{
		auto* fn = new ParsedFunction();
		fn->functionModule = file->fileModule;
		fn->functionIsVirtual = isVirtual;
		fn->isOverride = isOverride;
		fn->isAsync = isAsync;
		fn->scanDeclaration(currentLine, classStream, file, errors);
		fn->inClass = this;

		if (fn->name == "new")
		{
			fn->returnType = thisType;
		}
		else if (fn->name == "delete")
		{
			if (fn->returnTypeTokens.size())
			{
				errors->error(ErrorCode::parseInvalidType, fn->returnTypeTokens[0], "The delete function cannot have a return value.");
			}
			this->usedDestructor = fn;
		}


		fn->attributes = currentAttributes;
		currentAttributes.clear();
		this->methods.push_back(fn);
		return true;
	}

	errors->error(ErrorCode::parseUnexpectedToken,
		next, "Unexpected '" + next.string + "' in class definition");

	return true;
}

Function* ds::ParsedClass::getDefaultConstructor()
{
	if (this->thisType->constructors.empty())
	{
		return &this->constructor;
	}

	for (auto& i : this->thisType->constructors)
	{
		if (i->getArguments().empty())
		{
			return i;
		}
	}
	return &this->constructor;
}

void ds::ParsedClass::compileDestructor(ParseContext* context, ErrorContext* errors, ParsedFile* file)
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

		destructorScope.code->addBuffer(destructorScope.compileScopeExit(0, false));
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

ExpressionResult ds::ClassLifetimeFunction::compileCall()
{
	ExpressionResult result;
	result.code.addNew<BytecodeCallFunction>(getFullName());
	result.type = parent->thisType;
	result.valid = true;
	return result;
}

std::vector<FunctionArgument> ds::ClassLifetimeFunction::getArguments()
{
	return {};
}

std::string ds::ClassLifetimeFunction::getShortName() const
{
	return std::string();
}

std::string ds::ClassLifetimeFunction::getFullName() const
{
	return this->parent->classModule->name + "::" + this->parent->name.string + (this->isConstructor ? ".new.base" : ".delete.base");
}

bool ds::ClassLifetimeFunction::discardable() const
{
	return true;
}

ds::ParsedClass::~ParsedClass()
{
	delete thisType;
	for (ParsedFunction* i : this->methods)
	{
		delete i;
	}
}

void ds::ParsedClass::registerType(ParseContext* context, ParsedFile* file)
{
	thisType = new ClassType();
	thisType->from = name;
	thisType->name = name.string;
	thisType->nullable->name = name.string + "?";
	thisType->languageClass = this;
	file->fileModule->moduleTypes.insert({ name.string, thisType });
}

void ds::ParsedClass::scanClass(ParseContext* context, ParsedFile* file)
{
	if (scanned)
	{
		return;
	}

	scanned = true;
	uint32_t position = 0;
	std::vector<ClassMember> members;
	std::map<std::string, Function*> methods;

	std::vector<ClassType*> parents;
	for (auto& i : this->derivedFrom)
	{
		TokenLine line;
		line.lineTokens = &i;

		Type* type = file->getType(line, &context->errors);

		if (!type)
		{
			context->errors.error(ErrorCode::parseExpectedName, line.peek(),
				"Expected a type name, got " + line.peek().string);
			continue;
		}

		auto classType = dynamic_cast<ClassType*>(type);

		if (!classType)
		{
			context->errors.error(ErrorCode::parseInvalidType, line.previous(),
				"Cannot inherit from '" + Type::toString(type) + "', it isn't a class.");
			continue;
		}

		if (classType->languageClass)
		{
			classType->languageClass->scanClass(context, file);
			for (auto& m : classType->languageClass->members)
			{
				auto [value, success] = this->members.insert(m);
				value->second.isDerived = true;
			}
		}
		for (auto& i : classType->members)
		{
			members.push_back(i);
		}

		for (auto& i : classType->methods)
		{
			methods.insert(i);
		}
		position += Size(classType->classSize);

		parents.push_back(classType);
	}


	this->classModule = file->fileModule;

#ifdef WITH_LANGUAGE_SERVICE
	if (context->service)
	{
		context->service->files[file->name].types.push_back(this->name);
	}
#endif

	for (auto& m : this->members)
	{
		if (m.second.isDerived)
		{
			continue;
		}

		m.second.resolveAttributes(file, &context->errors);

#ifdef WITH_LANGUAGE_SERVICE
		if (context->service)
		{
			context->service->files[file->name].variables.push_back(m.first);
		}
#endif

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

#ifdef WITH_LANGUAGE_SERVICE
		if (context->service)
		{
			context->service->files[file->name]
				.functions.push_back(ScannedFunction(m, m->name));
		}
#endif
	}
}

void ds::ParsedClass::compile(ParseContext* context, ErrorContext* errors, ParsedFile* file)
{
	ParsedScope constructorScope;
	constructorScope.scopeFile = file;
	constructorScope.context = context;
	constructorScope.code = &this->constructor.code;
	auto& code = constructorScope.code;

	for (auto& i : thisType->parents)
	{
		if (i->baseConstructor)
			code->addBuffer(i->baseConstructor->compileCall().code);
	}

	constructorScope.setClass(this, true);

	for (auto& [name, member] : this->members)
	{
		if (member.isDerived)
			continue;
		if (member.value.empty())
		{
			if (member.type->hasDefaultValue)
			{
				continue;
			}
			else
			{
				errors->error(ErrorCode::parseInvalidType, member.name,
					"The type '" + Type::toString(member.type) + "' requires an initial value.");
				continue;
			}
		}

		TokenLine valueLine;
		valueLine.lineTokens = &member.value;
		auto varExpr = Expression::pushExpression(valueLine, &context->errors, false, member.type,
			&constructorScope);
		varExpr.compileToType(member.name, member.type, &constructorScope, errors);
		if (varExpr.type)
		{
			code->addBuffer(varExpr.code);
			code->addBuffer(varExpr.type->compileMove(&constructorScope));
			code->addBuffer(constructorScope.thisVariable->readValue(&constructorScope));

			code->pushInt(member.offset);
			code->pushInt(member.type->size);

			code->addOperation(BytecodeOp::setClassMember);
			valueLine.expectEndOfLine(errors);
		}
	}

	compileDestructor(context, errors, file);

	for (auto& i : this->methods)
	{
		if (i->name == "new")
		{
			// do not pop the return value of the base constructor, so we still have
			// a pointer to this
			i->functionCode.addNew<BytecodeCallFunction>(this->constructor.getFullName());
		}
		i->compile(context, file, errors);

		auto& bytecodeFunction = context->compiler.functions[i->getFullName()];
		bytecodeFunction.instructions = i->functionCode.instructions;
	}

	// Return a reference to this.
	code->addBuffer(constructorScope.thisVariable->readValue(&constructorScope));
	code->addBuffer(constructorScope.compileScopeExit(0, false));
	code->addOperation(BytecodeOp::ret);

	auto& constructorBytecode = context->compiler.functions[constructor.getFullName()];
	constructorBytecode.instructions = constructor.code.instructions;

	this->thisType->vTableOffset = bytecodeOffset(context->virtualTable.size());
	bytecodeOffset it = 1;
	context->virtualTable.push_back(usedDestructor);

	for (auto& p : thisType->parents)
	{
		for (auto& m : p->methods)
		{
			if (!m.second->isVirtual())
			{
				continue;
			}

			bool found = false;

			for (auto& i : this->methods)
			{
				if (!i->isOverride || i->getShortName() != m.second->getShortName())
				{
					continue;
				}
				if (!Function::signaturesMatch(i, m.second))
				{
					errors->error(ErrorCode::parseInvalidOverride, i->name,
						"Function signatures of " + i->getFullName() + " and " +
							m.second->getFullName() + " do not match.\nExpected signature: " +
							m.second->getSignatureText() + "\nGot:                " + i->getSignatureText());
				}

				context->virtualTable.push_back(i);
				i->vTableOffset = it++;
				i->foundOverride = true;
				it++;
				found = true;
				break;
			}
			if (found)
			{
				continue;
			}
			context->virtualTable.push_back(m.second);
			it++;
		}
	}

	for (auto& i : this->methods)
	{
		if (!i->foundOverride && i->isOverride)
		{
			errors->error(ErrorCode::parseInvalidOverride, i->name,
				"Could not find any function that " + i->name.string + " can override.");
		}
		if (i->functionIsVirtual && !i->isOverride)
		{
			i->vTableOffset = it++;
			context->virtualTable.push_back(i);
		}
	}
}

void ds::ParsedClass::scan(ErrorContext* errors, ParsedFile* file)
{
	this->usedDestructor = &baseDestructor;
	std::vector<AttribInfo> currentAttributes;
	while (scanLine(currentAttributes, errors, file))
	{
	}
}

BytecodeBuffer ds::ParsedClassMember::readValue() const
{
	BytecodeBuffer out;
	out.pushInt(offset);
	out.pushInt(type->size);

	out.addOperation(BytecodeOp::classMember);
	return out;
}

BytecodeBuffer ds::ParsedClassMember::writeValue() const
{
	BytecodeBuffer out;
	out.pushInt(offset);
	out.pushInt(type->size);

	out.addOperation(BytecodeOp::setClassMember);
	return out;
}
