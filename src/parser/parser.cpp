#include <parser/parser.hpp>
#include <parser/parseScope.hpp>
#include "language.hpp"
#include <modules/system.hpp>
using namespace lang;

lang::ParseContext::ParseContext(LanguageContext* context)
{
	for (auto& [name, module] : context->languageModules)
	{
		this->programModules.insert({ name, module->create() });
	}
}

void lang::ParseContext::addFile(std::string string)
{
	this->errors.currentFile = string;
	auto& newFile = this->files.emplace_back();
	newFile.stream.fromFile(string, &errors);
	newFile.name = string;
	newFile.scan(&errors);
}

BytecodeStream lang::ParseContext::compile()
{
	this->defaultTypes.push_back(new IntType());
	this->defaultTypes.push_back(new FloatType());

	scanModules();

	if (!errors.isOk())
	{
		return BytecodeStream();
	}

	for (auto& i : this->files)
	{
		this->errors.currentFile = i.name;
		i.compile(this);
	}

	if (!errors.isOk())
	{
		return BytecodeStream();
	}

	BytecodeStream out;
	this->compiler.compileTo(out);
	this->compiler.printAssembly();
	return out;
}

void lang::ParseContext::scanModules()
{
	// Register all modules
	for (ParsedFile& file : this->files)
	{
		Module& mod = this->programModules[file.scopeName];
		mod.name = file.scopeName;
		file.fileModule = &mod;

		for (auto& function : file.functions)
		{
			mod.moduleFunctions.insert({ function.name.string, &function });
			function.functionModule = &mod;
		}

		for (Type* i : this->defaultTypes)
		{
			mod.moduleTypes.insert({ i->name, i });
		}
	}

	// Resolve module dependencies
	for (ParsedFile& file : this->files)
	{
		for (auto& [name, module] : file.usings)
		{
			auto foundModule = this->programModules.find(name.string);
			if (foundModule == this->programModules.end())
			{
				errors.error(ErrorCode::parseUnknownModule, name, "Unknown module: " + name.string);
				continue;
			}
			module = &foundModule->second;
		}
	}

	// Parse classes in modules
	for (ParsedFile& file : this->files)
	{
		for (auto& i : file.classes)
		{
			i.scan(&errors, &file);
		}
	}

	// Parse functions in modules
	for (ParsedFile& file : this->files)
	{
		for (auto& i : file.classes)
		{
			i.registerType(this, &file);
		}

		this->errors.currentFile = file.name;
		for (auto& function : file.functions)
		{
			function.resolveTypes(this, &errors);
		}
	}
}

void lang::ParsedFile::loadAvailableTypes(ParseContext* context)
{
}

void lang::ParsedFile::scan(ErrorContext* errors)
{
	std::vector<AttribInfo> currentAttributes;
	while (scanLine(currentAttributes, errors)) {}
}

bool lang::ParsedFile::scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors)
{
	TokenLine currentLine;

	currentLine = stream.next(errors);

	if (currentLine.empty())
		return false;

	Token first = currentLine.get();

	// Module declaration
	if (first == "module")
	{
		this->addAttributes(currentAttributes);
		currentAttributes.clear();
		this->scopeName = currentLine.get().string;
		return true;
	}

	// Using other module
	if (first == "using")
	{
		this->usings[currentLine.get()] = nullptr;
		return true;
	}

	// Attribute
	if (first == "[")
	{
		currentAttributes.push_back(AttribInfo(currentLine.getUntil("]", errors)));
		return true;
	}

	if (first == "fn")
	{
		scanFunction(currentLine, errors).addAttributes(currentAttributes);
		currentAttributes.clear();
		return true;
	}

	if (first == "class")
	{
		scanClass(currentLine, errors).addAttributes(currentAttributes);
		currentAttributes.clear();
		return true;
	}

	errors->error(ErrorCode::parseUnexpectedToken, first, "Unexpected '" + first.string + "'");

	return true;
}

ParsedFunction& lang::ParsedFile::scanFunction(TokenLine currentLine, ErrorContext* errors)
{
	ParsedFunction& fn = this->functions.emplace_back();

	fn.name = currentLine.get();
	fn.functionFile = this;

	fn.argumentTokens = currentLine.getInBraces();

	Token next = currentLine.get();

	// If the brace is directly after the function arguments, it has no return
	// type.
	if (next == "{")
	{
		fn.start = next.position;
	}
	// Else it's the return type
	else if (next == "->")
	{
		fn.returnTypeTokens = currentLine.getUntil("{", errors);

		next = currentLine.get();

		if (next != "{")
		{
			errors->error(ErrorCode::parseUnexpectedToken, next,
				"Unexpected '" + next.string + "' after function definition. Expected '{'");
		}
	}
	else
	{
		errors->error(ErrorCode::parseUnexpectedToken, next,
			"Unexpected '" + next.string + "' after function definition. Expected '-> [type]' or '{'");
	}
	fn.start = next.position;

	this->stream.getScope(fn.functionStream, errors);

	return fn;
}

ParsedClass& lang::ParsedFile::scanClass(TokenLine currentLine, ErrorContext* errors)
{
	ParsedClass& newClass = this->classes.emplace_back();
	newClass.name = currentLine.get();

	stream.getScope(newClass.classStream, errors, 1);

	return newClass;
}

void lang::ParsedFile::compile(ParseContext* context)
{
	for (auto& fn : this->functions)
	{
		fn.compile(context, this, &context->errors);
	}

	for (auto c : this->classes)
	{
		c.compile(context, &context->errors, this);
	}
}

void lang::ParsedFunction::compile(ParseContext* context, ParsedFile* file, ErrorContext* errors)
{
	ParsedScope functionScope;
	functionScope.tokenStream = &functionStream;
	functionScope.code = &functionCode;
	functionScope.scopeFunction = this;
	functionScope.scopeFile = file;

	// Read the arguments given to the function and add them as variables in the scope.
	// They're added in reverse order because they're pushed onto the stack in this order.
	for (auto it = arguments.rbegin(); it < arguments.rend(); it++)
	{
		functionScope.pushVariableValue(it->type);
		functionScope.addVariable(it->name, it->type);
	}

	functionScope.compile(context, file, errors);

	auto& bytecodeFunction = context->compiler.functions[getFullName()];
	bytecodeFunction = functionCode;
	bytecodeFunction.isEntryPoint = getAttribute<modules::system::EntryPointAttribute>();
}

std::string lang::ParsedFunction::getFullName() const
{
	return this->functionModule->name + "::" + this->name.string;
}

void lang::ParsedFunction::resolveTypes(ParseContext* context, ErrorContext* errors)
{
	TokenLine line;

	if (!argumentTokens.empty())
	{
		line.lineTokens = &argumentTokens;

		while (true)
		{
			FunctionArgument newArgument;
			newArgument.type = functionModule->getType(line);
			newArgument.name = line.get();

			if (!newArgument.type)
			{
				errors->error(ErrorCode::parseUnknownSymbol, line.lineTokens->at(0),
					"Unknown type for function argument: '" + newArgument.name.string + "': '" +
						line.lineTokens->at(0).string + "'");
			}

			this->arguments.push_back(newArgument);

			if (line.empty())
				break;
			else if (line.get() != ",")
				errors->error(ErrorCode::parseUnexpectedToken, line.previous(),
					"Expected ',' or ')', got '" + line.previous().string + "'");
		}
	}

	if (!returnTypeTokens.empty())
	{
		line = TokenLine(&returnTypeTokens);

		this->returnType = functionModule->getType(line);

		if (!returnType)
		{
			errors->error(ErrorCode::parseUnknownSymbol, line.lineTokens->at(0),
				"Unknown return type for function '" + this->getFullName() + "': '" + line.lineTokens->at(0).string +
					"'");
		}
	}

	for (auto& i : this->attributes)
	{
		line = TokenLine(&i.attributeTokens);
		i.attribute = this->functionFile->getAttribute(line);
		if (!i.attribute)
		{
			errors->error(ErrorCode::parseUnknownSymbol, line.lineTokens->at(0),
				"Unknown attribute '" + line.lineTokens->at(0).string + "'");
		}
	}
}

ExpressionResult lang::ParsedFunction::compileCall()
{
	ExpressionResult result;
	result.code.add(new BytecodeCallFunction(getFullName()));
	result.type = returnType;
	result.valid = true;
	return result;
}

std::vector<FunctionArgument> lang::ParsedFunction::getArguments()
{
	return this->arguments;
}

bool lang::ParsedFunction::discardable() const
{
	return getAttribute<modules::system::DiscardAttribute>();
}

Function* ParsedFile::getMethod(std::string name)
{
	Function* found = this->fileModule->getMethod(name);
	if (found)
		return found;

	for (auto& i : this->usings)
	{
		found = i.second->getMethod(name);
		if (found)
			return found;
	}
	return nullptr;
}

Type* ParsedFile::getType(TokenLine& from)
{
	auto pos = from.savePosition();
	Type* found = this->fileModule->getType(from);
	if (found)
		return found;
	for (auto& i : this->usings)
	{
		from.loadPosition(pos);
		found = i.second->getType(from);
		if (found)
			return found;
	}
	from.loadPosition(pos);
	return nullptr;
}

Attribute* ParsedFile::getAttribute(TokenLine& from)
{
	auto pos = from.savePosition();
	Attribute* found = this->fileModule->getAttribute(from);
	if (found)
		return found;
	for (auto& i : this->usings)
	{
		from.loadPosition(pos);
		found = i.second->getAttribute(from);
		if (found)
			return found;
	}
	return nullptr;
}