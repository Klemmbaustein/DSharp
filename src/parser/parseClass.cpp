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
		name.checkIsName(errors);

		std::vector<Token> value;

		if (!currentLine.empty())
		{
			if (!currentLine.expect("=", errors))
			{
				value = currentLine.getUntil("", errors);

				if (value.empty())
				{
					errors->error(ErrorCode::parseUnexpectedEof, currentLine.previous(), "Expected a value after '='");
				}
			}
		}
		addMember(name, type, value, false).attributes = currentAttributes;
		currentAttributes.clear();

		return true;
	}

	currentLine.position = 0;

	bool isVirtual = false;
	bool isOverride = false;
	bool isAsync = false;
	std::vector<Token> modifiers;

	Token next = currentLine.get();

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
				errors->error(ErrorCode::parseInvalidType, fn->returnTypeTokens[0],
					"The delete function cannot have a return value.");
			}
			this->usedDestructor = fn;
		}


		fn->attributes = currentAttributes;
		currentAttributes.clear();
		this->methods.push_back(fn);
		return true;
	}

	if (isFileClass)
	{
		if (next == "using")
		{
			auto& usingName = currentLine.get();

			if (usingName.checkIsName(errors))
			{
				file->usings[usingName.string] = nullptr;
				file->updateUsings();
			}
			return true;
		}
	}

	errors->error(ErrorCode::parseUnexpectedToken,
		next, "Unexpected '" + next.string + "' in class definition");

	return true;
}

void ds::ParsedClass::scanDerived(BytecodeOffset& position, std::vector<ClassMember>& members,
	std::map<std::string, ClassMethod>& methods, ParseContext* context, ParsedFile* file)
{
	if (this->isInterface && !this->derivedFrom.empty())
	{
		context->errors.error(ErrorCode::parseInvalidType, this->name, "Interface cannot inherit from other types.");
		return;
	}

	for (auto& i : this->derivedFrom)
	{
		TokenLine line;
		line.lineTokens = &i;

		bool isInterface = false;

		if (line.peek() == "is")
		{
			isInterface = true;
			line.get();
		}

		Type* type = file->getType(line, &context->errors);

		if (!type)
		{
			context->errors.error(ErrorCode::parseExpectedName, line.peek(),
				"Expected a type name, got " + line.peek().string);
			continue;
		}

		ClassType* classType = type->asClass();

		if (!classType)
		{
			context->errors.error(ErrorCode::parseInvalidType, line.previous(),
				"Cannot inherit from '" + Type::toString(type) + "', it isn't a class.");
			continue;
		}

		if (classType->languageClass)
		{
			classType->languageClass->scanClass(context, classType->languageClass->definitionFile);
			for (auto& m : classType->languageClass->members)
			{
				auto [value, success] = this->members.insert(m);
				value->second.isDerived = true;
			}
		}

		for (auto& j : classType->interfaces)
		{
			this->thisType->interfaces.insert(j);
		}

		if (isInterface && !classType->isInterface)
		{
			context->errors.error(ErrorCode::parseInvalidType, *i.begin(),
				"Expected an interface, got " + Type::toString(classType));
		}
		else if (!isInterface && classType->isInterface)
		{
			context->errors.error(ErrorCode::parseInvalidType, *i.begin(),
				"A non interface class, got " + Type::toString(classType));
		}


		if (classType->isInterface)
		{
			thisType->interfaces.insert({ position, classType });
			position += Size(sizeof(RuntimeClass));
		}
		else
		{
			if (thisType->parent)
			{
				context->errors.error(ErrorCode::parseInvalidType, *i.begin(),
					"Can only derive from a single class. First class is " + Type::toString(thisType->parent));
			}
			thisType->parent = classType;
		}

		for (ClassMember j : classType->members)
		{
			j.offset += position;
			j.source = classType;
			members.push_back(j);
		}

		for (auto& j : classType->methods)
		{
			ClassMethod m = j.second;
			if (classType->isInterface)
				m.interfaceSource = classType;
			methods.insert({ j.first, m });
		}
		position += Size(classType->classSize);
	}
}

ParsedClassMember& ds::ParsedClass::addMember(Token name, ds::Type* type,
	const std::vector<ds::Token>& valueTokens, bool builtIn)
{
	if (builtIn)
	{
		auto [newMember, added] = this->builtInMembers.insert({ name,
			ParsedClassMember{
				.type = type,
				.name = name,
				.value = valueTokens,
			} });

		return newMember->second;
	}
	auto [newMember, added] = this->members.insert({ name,
		ParsedClassMember{
			.type = type,
			.name = name,
			.value = valueTokens,
		} });

	return newMember->second;
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
	BytecodeBuffer destructorCode;
	ParsedScope destructorScope;
	destructorScope.scopeFile = file;
	destructorScope.context = context;
	destructorScope.code = &destructorCode;
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
		auto& destructorBytecode = context->compiler.functions[baseDestructor.getFullName()];
		baseDestructor.code = &destructorBytecode;

		baseDestructor.code->addBuffer(destructorCode);
		baseDestructor.code->addBuffer(cleanupCode);

		baseDestructor.code->addBuffer(destructorScope.compileScopeExit(0, false));
		baseDestructor.code->addOperation(BytecodeOp::ret);
	}
	else
	{
		baseDestructor.code = nullptr;
		if (&baseDestructor == this->usedDestructor)
		{
			this->usedDestructor = nullptr;
			thisType->destructor = this->usedDestructor;
		}
	}
}

void ds::ParsedClass::compileBaseConstructor(ParseContext* context, ErrorContext* errors, ParsedFile* file)
{
	auto& constructorBytecode = context->compiler.functions[constructor.getFullName()];
	this->constructor.code = &constructorBytecode;

	ParsedScope constructorScope;
	constructorScope.scopeFile = file;
	constructorScope.context = context;
	constructorScope.code = this->constructor.code;
	auto& code = constructorScope.code;

	if (thisType->parent && thisType->parent->baseConstructor)
	{
		code->addBuffer(thisType->parent->baseConstructor->compileCall().code);
	}
	constructorScope.setClass(this, true);

	for (auto& [offset, interface] : thisType->interfaces)
	{
		code->addBuffer(constructorScope.thisVariable->readValue(&constructorScope));
		BinaryBuffer args;
		args.addValue(offset);
		args.addValue(createInterfaceVTable(interface, context, errors, file));
		code->addOperation(BytecodeOp::implInterface, args);
		if (interface->baseConstructor)
			code->addBuffer(interface->baseConstructor->compileCall().code);
		args.clear();
		args.addValue<Size>(sizeof(RuntimeClass*));
		code->addOperation(BytecodeOp::pop, args);
	}

	for (auto& [name, member] : this->members)
	{
		if (member.isDerived)
		{
			continue;
		}
		if (member.value.empty())
		{
			if (!member.type->hasDefaultValue)
			{
				errors->error(ErrorCode::parseInvalidType, member.name,
					"The type '" + Type::toString(member.type) + "' requires an initial value.");
			}
			continue;
		}

		TokenLine valueLine;
		valueLine.lineTokens = &member.value;
		auto varExpr = Expression::pushExpression(valueLine, &context->errors, false, member.type,
			&constructorScope);
		valueLine.expectEndOfLine(errors);
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

	// Return a reference to this.
	code->addBuffer(constructorScope.thisVariable->readValue(&constructorScope));
	code->addBuffer(constructorScope.compileScopeExit(0, false));
	code->addOperation(BytecodeOp::ret);

	compileConstructor(context, errors, file);
}

void ds::ParsedClass::compileConstructor(ParseContext* context, ErrorContext* errors, ParsedFile* file)
{
	if (thisType->parent && !thisType->parent->allowDirectConstructorCall)
	{
		thisType->allowDirectConstructorCall = false;
	}

	for (auto& i : this->methods)
	{
		i->registerFunction(context);
		if (i->name == "new")
		{
			// do not pop the return value of the base constructor, so we still have
			// a pointer to this
			i->functionCode->addNew<BytecodeCallFunction>(this->constructor.getFullName());
			if (thisType->parent)
			{
				for (auto& c : thisType->parent->constructors)
				{
					if (c->getArguments().empty())
					{
						i->functionCode->addBuffer(c->compileCall().code);
						break;
					}
				}
			}
			for (auto& [_, p] : this->thisType->interfaces)
			{
				for (auto& c : p->constructors)
				{
					if (c->getArguments().empty())
					{
						i->functionCode->addBuffer(c->compileCall().code);
						break;
					}
				}
			}
		}
		i->compile(context, file, errors);
	}
}

void ds::ParsedClass::handleParentClass(BytecodeOffset& vTableIndex, ClassType* parent, ParseContext* context,
	ErrorContext* errors, ParsedFile* file)
{
	for (auto& m : parent->methods)
	{
		if (!m.second.function->isVirtual())
		{
			continue;
		}

		if (m.second.interfaceSource != nullptr)
		{
			continue;
		}

		bool found = false;

		for (auto& i : this->methods)
		{
			if (!i->isOverride || i->getShortName() != m.second.function->getShortName())
			{
				continue;
			}
			if (!Function::signaturesMatch(i, m.second.function))
			{
				errors->error(ErrorCode::parseInvalidOverride, i->name,
					"Function signatures of " + i->getFullName() + " and " +
						m.second.function->getFullName() + " do not match.\nExpected signature: " +
						m.second.function->getSignatureText() + "\nGot:                " + i->getSignatureText());
			}

			context->virtualTable.push_back(i);
			i->vTableOffset = vTableIndex++;
			i->foundOverride = true;
			found = true;
			break;
		}
		if (found)
		{
			continue;
		}
		context->virtualTable.push_back(m.second.function);
		vTableIndex++;
	}
}

BytecodeOffset ds::ParsedClass::createInterfaceVTable(ClassType* interface, ParseContext* context,
	ErrorContext* errors, ParsedFile* file)
{
	BytecodeOffset vTableIndex = 1;
	BytecodeOffset initialIndex = BytecodeOffset(context->virtualTable.size());

	context->virtualTable.push_back(usedDestructor);

	for (auto& m : interface->methods)
	{
		if (!m.second.function->isVirtual())
		{
			continue;
		}

		bool found = false;

		for (auto& i : this->methods)
		{
			if (!i->isOverride || i->foundOverride || i->getShortName() != m.second.function->getShortName())
			{
				continue;
			}
			if (!Function::signaturesMatch(i, m.second.function))
			{
				errors->error(ErrorCode::parseInvalidOverride, i->name,
					"Function signatures of " + i->getFullName() + " and " +
						m.second.function->getFullName() + " do not match.\nExpected signature: " +
						m.second.function->getSignatureText() + "\nGot:                " + i->getSignatureText());
			}

			context->virtualTable.push_back(i);

			BinaryBuffer args;
			args.addValue<Int>(thisType->getInterfaceOffset(interface));
			args.addValue<Bool>(true);

			i->functionCode->instructions.insert(i->functionCode->instructions.begin(),
				std::make_shared<BytecodeOperation>(BytecodeOp::castInterface, args));
			i->vTableOffset = vTableIndex++;
			i->foundOverride = true;
			found = true;
			break;
		}
		if (found)
		{
			continue;
		}
		context->virtualTable.push_back(thisType->methods.at(m.first).function);
		vTableIndex++;
	}

	return initialIndex;
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
	return getFullName();
}

std::string ds::ClassLifetimeFunction::getFullName() const
{
	return this->parent->classModule->name + "::" +
	       this->parent->name.string +
	       (this->isConstructor ? ".new.base" : ".delete.base");
}

bool ds::ClassLifetimeFunction::discardable() const
{
	return true;
}

ds::ParsedClass::~ParsedClass()
{
	clearMethods();
	delete thisType;
}

void ds::ParsedClass::clearMethods()
{
	this->constructor = ClassLifetimeFunction();
	this->baseDestructor = ClassLifetimeFunction();
	for (ParsedFunction* i : this->methods)
	{
		delete i;
	}
	methods.clear();
}

void ds::ParsedClass::registerType(ParseContext* context, ParsedFile* file)
{
	if (thisType)
	{
		file->fileModule->moduleTypes.insert({ name.string, thisType });
		return;
	}

	thisType = new ClassType();
	thisType->from = name;
	thisType->module = file->moduleName;
	thisType->name = name.string;
	thisType->nullable->name = name.string + "?";
	thisType->languageClass = this;
	thisType->applyName();
	thisType->nullable->applyName();
	thisType->isInterface = this->isInterface;
	if (file->fileModule)
		file->fileModule->moduleTypes.insert({ name.string, thisType });
}

void ds::ParsedClass::scanClass(ParseContext* context, ParsedFile* file)
{
	if (scanned)
	{
		return;
	}

	scanned = true;
	BytecodeOffset position = 0;
	std::vector<ClassMember> members;
	std::map<std::string, ClassMethod> methods;

	scanDerived(position, members, methods, context, file);

	this->classModule = file->fileModule;

#ifdef WITH_LANGUAGE_SERVICE
	if (context->service)
	{
		context->service->files[file->name].types.push_back(ScannedTypeUsage(this->name, thisType));
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
			context->service->files[file->name].variables.push_back(
				ScannedVariable(&m.second, this->thisType, file, m.first));
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
			auto found = methods.find(i->name.string);
			if (found != methods.end())
			{
				found->second = ClassMethod(i, found->second.interfaceSource);
			}
			else
			{
				methods.insert({ i->name.string, ClassMethod(i) });
			}
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

	for (auto& m : this->methods)
	{
		m->resolveTypes(context, &context->errors);

#ifdef WITH_LANGUAGE_SERVICE
		if (context->service)
		{
			context->service->files[file->name]
				.functions.push_back(ScannedFunction(m, m->name, ScannedFunction::Kind::functionDefinition));
		}
#endif
	}
}

void ds::ParsedClass::compile(ParseContext* context, ErrorContext* errors, ParsedFile* file)
{
	compileDestructor(context, errors, file);

	this->thisType->vTableOffset = BytecodeOffset(context->virtualTable.size());
	BytecodeOffset vTableIndex = 1;
	context->virtualTable.push_back(usedDestructor);

	if (thisType->parent)
	{
		handleParentClass(vTableIndex, thisType->parent, context, errors, file);
	}

	for (auto& i : this->methods)
	{
		i->registerFunction(context);
		if (i->functionIsVirtual && !i->isOverride)
		{
			i->vTableOffset = vTableIndex++;
			context->virtualTable.push_back(i);
		}
	}
	compileBaseConstructor(context, errors, file);

	for (auto& i : this->methods)
	{
		if (!i->foundOverride && i->isOverride)
		{
			errors->error(ErrorCode::parseInvalidOverride, i->name,
				"Could not find any function that " + i->name.string + " can override.");
		}
	}
}

void ds::ParsedClass::scan(ErrorContext* errors, ParsedFile* file)
{
	this->members = this->builtInMembers;
	this->clearMethods();

	thisType->parent = nullptr;
	thisType->interfaces.clear();

	this->classStream.reset();
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
