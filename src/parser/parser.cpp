#include <parser/parser.hpp>
#include <parser/parseScope.hpp>
#include <parser/bytecode/compileBytecodeVirtual.hpp>
#include <parser/types/stringType.hpp>
#include <parser/types/listType.hpp>
#include <modules/system.hpp>
#include <language.hpp>
using namespace lang;

lang::ParseContext::ParseContext(LanguageContext* context)
{
	for (auto& [name, module] : context->languageModules)
	{
		this->programModules.insert({ name, module->create() });
	}

	for (auto& i : this->programModules)
	{
		std::string moduleName = i.first + "::";

		for (auto& m : this->programModules)
		{
			if (m.first.substr(0, moduleName.size()) == moduleName)
			{
				i.second.submodules.insert({ m.first, &m.second });
			}
		}
	}
}

lang::ParseContext::~ParseContext()
{
}

void lang::ParseContext::addFile(std::string filePath)
{
	this->errors.currentFile = filePath;
	auto& newFile = this->files.emplace_back();
	newFile.stream.fromFile(filePath, &errors);
	newFile.name = filePath;
	newFile.scan(&errors);
}

BytecodeStream lang::ParseContext::compile()
{
	this->defaultTypes.push_back(IntType::getInstance());
	this->defaultTypes.push_back(FloatType::getInstance());
	this->defaultTypes.push_back(BoolType::getInstance());
	this->defaultTypes.push_back(StringType::getInstance());
	this->defaultTypes.push_back(CharType::getInstance());
	this->defaultTypes.push_back(ListType::getInstance());
	this->defaultTypes.push_back(NullType::getInstance());

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
	this->compiler.compileTo(out, virtualTable, &errors);
	//this->compiler.printAssembly();
	if (!errors.isOk())
	{
		return BytecodeStream();
	}
	return out;
}

void lang::ParseContext::scanModules()
{
	// Register all modules
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
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

	// Create global module
	auto& globalModule = this->programModules.insert({ "", Module() }).first->second;

	for (auto& i : this->programModules)
	{
		if (i.second.name != globalModule.name)
		{
			globalModule.submodules.insert({ i.first, &i.second });
		}
	}

	// Resolve module dependencies
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
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
		file.usings.insert({ Token(""), &globalModule });
	}

	// Scan class types
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		for (auto& i : file.classes)
		{
			i.registerType(this, &file);
		}
	}

	// Parse classes in modules
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		for (auto& i : file.classes)
		{
			i.scan(&errors, &file);
		}
	}

	// Scan classes
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		for (auto& i : file.classes)
		{
			i.scanClass(this, &file);
		}
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
lang::ParsedFile::~ParsedFile()
{
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
		auto attribTokens = currentLine.getUntil("]", errors);

		if (attribTokens.empty())
		{
			errors->error(ErrorCode::parseUnexpectedToken, first, "Expected an attribute name after '['");
			return true;
		}

		currentAttributes.push_back(AttribInfo(attribTokens));
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

	fn.scanDeclaration(currentLine, stream, this, errors);

	return fn;
}

ParsedClass& lang::ParsedFile::scanClass(TokenLine currentLine, ErrorContext* errors)
{
	ParsedClass& newClass = this->classes.emplace_back();
	newClass.name = currentLine.get();
	newClass.name.checkIsName(errors);

	if (currentLine.peek() == ":")
	{
		currentLine.get();
		while (true)
		{
			newClass.derivedFrom.push_back(currentLine.getUntil("{,", errors));

			if (currentLine.previous() == "{")
			{
				break;
			}
			else if (currentLine.empty())
			{
				errors->error(ErrorCode::parseUnexpectedToken, currentLine.previous(), "Expected a '{'");
				break;
			}
			else if (currentLine.previous() != ",")
			{
				errors->error(ErrorCode::parseUnexpectedToken, currentLine.peek(), "Unexpected " + currentLine.peek().string);
				break;
			}
		}
	}
	else
	{
		auto found = currentLine.get();

		if (found != "{" && !found.empty())
		{
			errors->error(ErrorCode::parseUnexpectedToken, found, "Expected a '{', got: '" + found.string + "'");
		}
	}

	stream.getScope(newClass.classStream, errors, 1);

	return newClass;
}

void lang::ParsedFile::compile(ParseContext* context)
{
	for (auto& fn : this->functions)
	{
		fn.compile(context, this, &context->errors);
	}

	for (auto& c : this->classes)
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
	functionScope.compileReturn = true;
	functionScope.scopeFile = file;
	functionScope.returnThis = this->inClass && this->name == "new";

	if (this->inClass)
	{
		functionScope.setClass(this->inClass, this->name != "delete");
	}
	// Read the arguments given to the function and add them as variables in the scope.
	// They're added in reverse order because they're pushed onto the stack in this order.
	for (auto it = arguments.rbegin(); it < arguments.rend(); it++)
	{
		functionScope.pushVariableValue(it->type, false);
		functionScope.addVariable(it->name, it->type);
	}

	functionScope.compile(context, file, errors);

	auto& bytecodeFunction = context->compiler.functions[getFullName()];
	bytecodeFunction = functionCode;
	bytecodeFunction.isEntryPoint = getAttribute<modules::system::EntryPointAttribute>();
}

std::string lang::ParsedFunction::getFullName() const
{
	if (this->inClass)
		return this->functionModule->name + "::" + this->inClass->name.string + "." + this->name.string;
	return this->functionModule->name + "::" + this->name.string;
}

std::string lang::ParsedFunction::getShortName() const
{
	return this->name.string;
}

void lang::ParsedFunction::scanDeclaration(TokenLine currentLine, TokenStream& stream, ParsedFile* file, ErrorContext* errors)
{
	name = currentLine.get();
	functionFile = file;

	argumentTokens = currentLine.getInBraces(errors);

	Token next = currentLine.get();

	// If the brace is directly after the function arguments, it has no return
	// type.
	if (next == "{")
	{
		start = next.position;
	}
	// Else it's the return type
	else if (next == "->")
	{
		returnTypeTokens = currentLine.getUntil("{", errors);
	}
	else
	{
		errors->error(ErrorCode::parseUnexpectedToken, next,
			"Unexpected '" + next.string + "' after function definition. Expected '-> [type]' or '{'");
	}
	start = next.position;

	stream.getScope(functionStream, errors);
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
			newArgument.type = functionFile->getType(line);
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

		this->returnType = functionFile->getType(line);
		line.expectEndOfLine(errors);

		if (!returnType)
		{
			errors->error(ErrorCode::parseUnknownSymbol, line.lineTokens->at(0),
				"Unknown return type for function '" + this->getFullName() + "': '" + line.lineTokens->at(0).string +
					"'");
		}
	}

	for (auto& i : this->attributes)
	{
		i.attributeTokens[0].checkIsName(errors);

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
	if (this->functionIsVirtual)
	{
		result.code.addNew<BytecodeCallVirtual>(this, this->inClass->thisType);
	}
	else
	{
		result.code.addNew<BytecodeCallFunction>(getFullName());
	}
	result.type = returnType;
	result.valid = true;
	result.discardable = this->discardable();
	return result;
}

std::vector<FunctionArgument> lang::ParsedFunction::getArguments()
{
	return this->arguments;
}

Type* lang::ParsedFunction::getReturnType()
{
	return this->returnType;
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
	auto initialPos = from.savePosition();
	auto name = from.get();
	auto pos = from.savePosition();
	Type* found = this->fileModule->getType(name.string, from);
	if (found)
		return found;
	for (auto& i : this->usings)
	{
		from.loadPosition(pos);
		found = i.second->getType(name.string, from);
		if (found)
			return found;
	}
	from.loadPosition(initialPos);
	return nullptr;
}

Attribute* ParsedFile::getAttribute(TokenLine& from)
{
	auto initialPos = from.savePosition();
	auto name = from.get();
	auto pos = from.savePosition();
	Attribute* found = this->fileModule->getAttribute(name.string, from);
	if (found)
		return found;
	for (auto& i : this->usings)
	{
		from.loadPosition(pos);
		found = i.second->getAttribute(name.string, from);
		if (found)
			return found;
	}
	from.loadPosition(initialPos);
	return nullptr;
}