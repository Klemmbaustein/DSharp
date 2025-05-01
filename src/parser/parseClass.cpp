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

		this->members.push_back(ParsedClassMember{
			.type = type,
			.name = name,
			.value = value,
		});
		return true;
	}

	currentLine.position = 0;

	auto first = currentLine.get();

	if (first == "fn")
	{
		this->methods.push_back(new ParsedFunction());
	}

	errors->error(ErrorCode::parseUnexpectedToken,
		first, "Unexpected '" + first.string + "' in class definiton");

	return true;
}

ExpressionResult lang::ConstructorFunction::compileCall()
{
	ExpressionResult result;
	result.code.add(new BytecodeCallFunction(getFullName()));
	result.type = parent->thisType;
	result.valid = true;
	return result;
}

std::vector<FunctionArgument> lang::ConstructorFunction::getArguments()
{
	return {};
}

std::string lang::ConstructorFunction::getFullName() const
{
	return this->parent->classModule->name + "::" + this->parent->name.string + ".new";
}

bool lang::ConstructorFunction::discardable() const
{
	return true;
}

void lang::ParsedClass::registerType(ParseContext* context, ParsedFile* file)
{
	std::vector<ClassMember> members;

	this->classModule = file->fileModule;

	uint32_t position = 0;
	for (auto& m : this->members)
	{
		members.push_back(ClassMember{
			.name = m.name,
			.offset = position,
			.type = m.type,
		});
		m.offset = position,
		position += m.type->size;
	}

	constructor.parent = this;
	ParsedScope constructorScope;
	constructorScope.scopeFile = file;
	constructorScope.context = context;
	constructorScope.code = &this->constructor.code;
	thisType = new ClassType(name, members, position, &this->constructor);
	constructorScope.pushVariableValue(this->thisType);
	auto& thisVariable = constructorScope.addVariable(Token("this"), thisType);

	for (auto& i : this->members)
	{
		TokenLine valueLine = TokenLine(&i.value);
		auto varExpr = constructorScope.pushExpression(valueLine, &context->errors, false);
		constructorScope.code->addBuffer(varExpr.code);
		constructorScope.code->addBuffer(thisVariable.readValue(&constructorScope));

		BinaryBuffer args;
		args.addValue(i.offset);
		args.addValue(i.type->size);

		constructorScope.code->addOperation(BytecodeOp::setClassMember, args);
	}

	// Return a reference to this.
	constructorScope.code->addBuffer(thisVariable.readValue(&constructorScope));
	constructorScope.compileScopeExit(true);
	constructorScope.code->addOperation(BytecodeOp::ret);

	file->fileModule->moduleTypes.insert({ name.string, thisType });
}

void lang::ParsedClass::compile(ParseContext* context, ErrorContext* errors, ParsedFile* file)
{
	auto& bytecodeFunction = context->compiler.functions[constructor.getFullName()];
	bytecodeFunction.instructions = constructor.code.instructions;
}

void lang::ParsedClass::scan(ErrorContext* errors, ParsedFile* file)
{
	while (scanLine(errors, file))
	{
	}
}
