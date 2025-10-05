#include <ds/parser/parser.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/bytecode/compileBytecodeVirtual.hpp>
#include <ds/parser/types/stringType.hpp>
#include <ds/parser/types/listType.hpp>
#include <ds/parser/types/functionType.hpp>
#include <ds/parser/types/lambdaType.hpp>
#include <ds/modules/system.hpp>
#include <ds/language.hpp>
#include <ds/service/languageService.hpp>
using namespace ds;

ds::ParseContext::ParseContext(LanguageContext* context)
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

ds::ParseContext::~ParseContext()
{
}

void ds::ParseContext::addFile(std::string filePath)
{
	this->errors.currentFile = filePath;
	auto& newFile = this->files.emplace_back();
	newFile.stream.fromFile(filePath, &errors);
	newFile.name = filePath;
	newFile.context = this;
	newFile.scan(&errors);
}

void ds::ParseContext::addString(const std::string& str, std::string fileName)
{
	this->errors.currentFile = fileName;
	auto& newFile = this->files.emplace_back();
	newFile.stream.fromString(str, fileName, &errors);
	newFile.name = fileName;
	newFile.context = this;
	newFile.scan(&errors);
}

void ds::ParseContext::updateFile(const std::string& str, std::string fileName)
{
	for (auto& i : this->files)
	{
		if (i.name == fileName)
		{
			i.stream = TokenStream();
			i.stream.fromString(str, fileName, &errors);
			i.context = this;
			i.scan(&errors);
			break;
		}
	}
}

BytecodeStream ds::ParseContext::compile()
{
	virtualTable.clear();
	this->defaultTypes.clear();
	this->defaultTypes.push_back(IntType::getInstance());
	this->defaultTypes.push_back(FloatType::getInstance());
	this->defaultTypes.push_back(BoolType::getInstance());
	this->defaultTypes.push_back(StringType::getInstance());
	this->defaultTypes.push_back(CharType::getInstance());
	this->defaultTypes.push_back(ListType::getInstance());
	this->defaultTypes.push_back(NullType::getInstance());
	this->defaultTypes.push_back(FunctionType::getInstance(nullptr, {}));
	this->defaultTypes.push_back(LambdaType::getInstance());

#ifdef WITH_LANGUAGE_SERVICE
	if (this->service)
	{
		this->service->files.clear();
	}
#endif

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

	for (auto& i : this->files)
	{
		for (auto& cls : i.classes)
		{
			std::vector<TypeMember> members;

			for (auto& m : cls.members)
			{
				auto reflectAttribute = m.second.getAttribute<modules::system::ReflectAttribute>();

				if (reflectAttribute)
				{
					members.push_back(TypeMember{
						.type = 0,
						.name = m.second.name.string,
						.offset = m.second.offset,
						});
				}
			}

			out.reflect.types[0] = TypeInfo{
				.name = cls.classModule->name + "::" + cls.name.string,
				.vTableOffset = cls.thisType->vTableOffset,
				.constructor = this->compiler.functions[cls.getDefaultConstructor()->getFullName()].offset,
				.bodySize = cls.thisType->classSize,
				.members = members,
			};
		}
	}

	//if (!this->service)
	//{
	//	this->compiler.printAssembly();
	//}

	if (!errors.isOk())
	{
		return BytecodeStream();
	}
	return out;
}

void ds::ParseContext::scanModules()
{
	// Register all modules
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		Module& mod = this->programModules[file.scopeName];
		mod.submodules.clear();
		mod.moduleAttributes.clear();
		mod.moduleEnums.clear();
		mod.moduleFunctions.clear();
		mod.moduleTypes.clear();
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

	// Register types
	for (ParsedFile& file : this->files)
	{
		this->errors.currentFile = file.name;
		for (auto& i : file.classes)
		{
			i.registerType(this, &file);
		}
		for (auto& i : file.enums)
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

void ds::ParsedFile::loadAvailableTypes(ParseContext* context)
{
}

void ds::ParsedFile::scan(ErrorContext* errors)
{
	this->scopeName = "";
	this->usings.clear();
	this->enums.clear();
	this->attributes.clear();
	this->functions.clear();
	this->classes.clear();
	std::vector<AttribInfo> currentAttributes;
	while (scanLine(currentAttributes, errors)) {}
}
ds::ParsedFile::~ParsedFile()
{
}

bool ds::ParsedFile::scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors)
{
	TokenLine currentLine = stream.next(errors);

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

	if (first == "enum")
	{
		scanEnum(currentLine, errors).addAttributes(currentAttributes);
		currentAttributes.clear();
		return true;
	}

	errors->error(ErrorCode::parseUnexpectedToken, first, "Unexpected '" + first.string + "'");

	return true;
}

ParsedFunction& ds::ParsedFile::scanFunction(TokenLine currentLine, ErrorContext* errors)
{
	ParsedFunction& fn = this->functions.emplace_back();

	fn.scanDeclaration(currentLine, stream, this, errors);

	return fn;
}

ParsedClass& ds::ParsedFile::scanClass(TokenLine currentLine, ErrorContext* errors)
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
				return newClass;
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
			return newClass;
		}
	}

	stream.getScope(newClass.classStream, errors, 1);

	return newClass;
}

ParsedEnum& ds::ParsedFile::scanEnum(TokenLine currentLine, ErrorContext* errors)
{
	ParsedEnum& newEnum = this->enums.emplace_back();
	newEnum.name = currentLine.get();

	if (currentLine.get() != "{")
	{
		errors->error(ErrorCode::parseUnexpectedToken, currentLine.previous(), "Expected a '{'");
		return newEnum;
	}

	stream.getScope(newEnum.scope, errors, 1);
	return newEnum;
}

void ds::ParsedFile::compile(ParseContext* context)
{
#ifdef WITH_LANGUAGE_SERVICE
	ScannedFile* scanInfo = nullptr;

	if (context->service)
	{
		scanInfo = &context->service->files[this->name];
	}
#endif

	for (auto& c : this->classes)
	{
		c.compile(context, &context->errors, this);
	}

	for (auto& fn : this->functions)
	{
#ifdef WITH_LANGUAGE_SERVICE
		if (scanInfo)
		{
			scanInfo->functions.push_back(ScannedFunction(&fn, fn.name));
		}
#endif
		fn.compile(context, this, &context->errors);
	}
}

void ds::ParsedFunction::compile(ParseContext* context, ParsedFile* file, ErrorContext* errors)
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
		functionScope.addVariable(it->name, it->type, errors);
	}

	functionScope.compile(context, file, errors);

	auto& bytecodeFunction = context->compiler.functions[getFullName()];
	bytecodeFunction = functionCode;
	bytecodeFunction.isEntryPoint = getAttribute<modules::system::EntryPointAttribute>();
}

std::string ds::ParsedFunction::getFullName() const
{
	if (this->inClass)
		return this->functionModule->name + "::" + this->inClass->name.string + "." + this->name.string;
	return this->functionModule->name + "::" + this->name.string;
}

std::string ds::ParsedFunction::getShortName() const
{
	return this->name.string;
}

void ds::ParsedFunction::scanDeclaration(TokenLine currentLine, TokenStream& stream, ParsedFile* file, ErrorContext* errors)
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

void ds::ParsedFunction::resolveTypes(ParseContext* context, ErrorContext* errors)
{
	TokenLine line;

	if (!argumentTokens.empty())
	{
		line.lineTokens = &argumentTokens;

		while (true)
		{
			FunctionArgument newArgument;
			newArgument.type = functionFile->getType(line, errors);
			newArgument.name = line.get();

			if (!newArgument.type)
			{
				errors->error(ErrorCode::parseUnknownSymbol, line.lineTokens->at(0),
					"Unknown type for function argument: '" + newArgument.name.string + "': '" +
						line.lineTokens->at(0).string + "'");
			}

			this->arguments.push_back(newArgument);
#ifdef WITH_LANGUAGE_SERVICE
			if (context->service)
			{
				context->service->files[functionFile->name].variables.push_back(newArgument.name);
			}
#endif

			if (line.empty())
				break;
			else if (line.get() != ",")
				errors->error(ErrorCode::parseUnexpectedToken, line.previous(),
					"Expected ',' or ')', got '" + line.previous().string + "'");
		}
	}

	if (!returnTypeTokens.empty())
	{
		line = TokenLine();
		line.lineTokens = &returnTypeTokens;

		this->returnType = functionFile->getType(line, errors);
		line.expectEndOfLine(errors);

		if (!returnType)
		{
			errors->error(ErrorCode::parseUnknownSymbol, line.lineTokens->at(0),
				"Unknown return type for function '" + this->getFullName() + "': '" + line.lineTokens->at(0).string +
					"'");
		}
	}

	resolveAttributes(functionFile, errors);
}

ExpressionResult ds::ParsedFunction::compileCall()
{
	ExpressionResult result;
	if (this->functionIsVirtual)
	{
		result.code.addNew<BytecodeCallVirtual>(this);
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

std::vector<FunctionArgument> ds::ParsedFunction::getArguments()
{
	return this->arguments;
}

Type* ds::ParsedFunction::getReturnType()
{
	return this->returnType;
}

bool ds::ParsedFunction::discardable() const
{
	return getAttribute<modules::system::DiscardAttribute>();
}

BytecodeBuffer ds::ParsedFunction::compileCallable(ErrorContext* errors, ParsedScope* with, Type* hintType) const
{
	BytecodeBuffer result;
	result.addNew<BytecodeFunctionAddress>(getFullName());
	if (isLambda)
	{
		result.addNew<BytecodeCallNative>("system::fn.new.lambda");
	}
	else
	{
		result.addNew<BytecodeCallNative>("system::fn.new.bytecode");
	}
	return result;
}

Function* ParsedFile::getMethod(std::string name)
{
	Function* found = this->fileModule->getMethod(name);
	if (found)
		return found;

	for (auto& i : this->usings)
	{
		if (!i.second)
			continue;
		found = i.second->getMethod(name);
		if (found)
			return found;
	}
	return nullptr;
}

Type* ParsedFile::getType(TokenLine& from, ErrorContext* errors)
{
	auto initialPos = from.savePosition();
	auto name = from.get();
	auto pos = from.savePosition();
	Type* found = this->fileModule->getType(name, from, errors, this, this->context);
	if (found)
		return found;
	for (auto& i : this->usings)
	{
		if (!i.second)
			continue;
		from.loadPosition(pos);
		found = i.second->getType(name, from, errors, this, this->context);
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
	Attribute* found = this->fileModule->getAttribute(name, from, this, this->context);
	if (found)
		return found;
	for (auto& i : this->usings)
	{
		if (!i.second)
			continue;
		from.loadPosition(pos);
		found = i.second->getAttribute(name, from, this, this->context);
		if (found)
			return found;
	}
	from.loadPosition(initialPos);
	return nullptr;
}

std::pair<EnumType*, std::string> ds::ParsedFile::getEnum(TokenLine& from)
{
	auto initialPos = from.savePosition();
	auto name = from.get();
	auto pos = from.savePosition();
	auto found = this->fileModule->getEnum(name.string, from);
	if (found.first)
		return found;
	for (auto& i : this->usings)
	{
		if (!i.second)
			continue;
		from.loadPosition(pos);
		found = i.second->getEnum(name.string, from);
		if (found.first)
			return found;
	}
	from.loadPosition(initialPos);
	return { nullptr, "" };
}
