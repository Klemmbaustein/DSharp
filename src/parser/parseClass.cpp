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

	auto first = currentLine.get();

	if (first == "fn")
	{
		auto* fn = new ParsedFunction();
		fn->functionModule = file->fileModule;
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
		first, "Unexpected '" + first.string + "' in class definiton");

	return true;
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

void lang::ParsedClass::registerType(ParseContext* context, ParsedFile* file)
{
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
	auto thisClassType = new ClassType();
	thisClassType->from = name;
	thisClassType->name = name.string;
	thisClassType->members = members;
	thisClassType->methods = methods;
	thisClassType->classSize = position;
	thisClassType->baseConstructor = &this->constructor;
	thisClassType->destructor = this->usedDestructor;
	thisClassType->constructors = constructors;
	thisType = thisClassType;

	file->fileModule->moduleTypes.insert({ name.string, thisType });

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
	constructorScope.setClass(this);

	for (auto& [name, member] : this->members)
	{
		if (member.value.empty())
			continue;

		TokenLine valueLine = TokenLine(&member.value);
		auto varExpr = constructorScope.pushExpression(valueLine, &context->errors, false);
		constructorScope.code->addBuffer(varExpr.code);
		constructorScope.code->addBuffer(constructorScope.thisVariable->readValue(&constructorScope));

		constructorScope.code->pushInt(member.offset);
		constructorScope.code->pushInt(member.type->size);

		constructorScope.code->addOperation(BytecodeOp::setClassMember);
	}

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
	constructorScope.code->addBuffer(constructorScope.thisVariable->readValue(&constructorScope));
	constructorScope.compileScopeExit(true);
	constructorScope.code->addOperation(BytecodeOp::ret);


	auto& constructorBytecode = context->compiler.functions[constructor.getFullName()];
	constructorBytecode.instructions = constructor.code.instructions;

	if (usedDestructor == &baseDestructor)
	{
		auto& destructorBytecode = context->compiler.functions[baseDestructor.getFullName()];
		destructorBytecode.instructions = baseDestructor.code.instructions;
	}
}

void lang::ParsedClass::scan(ErrorContext* errors, ParsedFile* file)
{
	while (scanLine(errors, file))
	{
	}
}

BytecodeBuffer lang::ParsedClassMember::readValue() const
{
	BytecodeBuffer out;
	BinaryBuffer args;
	args.addValue(offset);
	args.addValue(type->size);

	out.addOperation(BytecodeOp::classMember, args);
	return out;
}

BytecodeBuffer lang::ParsedClassMember::writeValue() const
{
	BytecodeBuffer out;
	BinaryBuffer args;
	args.addValue(offset);
	args.addValue(type->size);

	out.addOperation(BytecodeOp::setClassMember, args);
	return out;
}
