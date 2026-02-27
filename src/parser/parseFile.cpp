#include <ds/parser/parseFile.hpp>
#include <ds/parser/parseEnum.hpp>
#include <ds/service/languageService.hpp>

using namespace ds;

void ds::ParsedFile::scan(ErrorContext* errors)
{
	this->moduleName = "";
	this->usings.clear();
	this->enums.clear();
	this->attributes.clear();
	this->functions.clear();
	this->classes.clear();
	std::vector<AttribInfo> currentAttributes;
	while (scanLine(currentAttributes, errors)) {}
}

void ds::ParsedFile::updateUsings()
{
	for (auto& [name, module] : usings)
	{
		if (module)
		{
			continue;
		}
		auto foundModule = context->programModules.find(name.string);
		if (foundModule == context->programModules.end())
		{
			context->errors.error(ErrorCode::parseUnknownModule, name, "Unknown module: " + name.string);
			continue;
		}
		module = &foundModule->second;
	}
}

bool ds::ParsedFile::scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors)
{
	TokenLine currentLine = stream.next(errors);

	if (currentLine.empty())
		return false;

	auto fn = scanFunction(currentLine, errors);

	if (fn)
	{
		fn->addAttributes(currentAttributes);
		currentAttributes.clear();
		return true;
	}

	Token first = currentLine.get();

	// Module declaration
	if (first == "module")
	{
		this->addAttributes(currentAttributes);
		currentAttributes.clear();
		this->moduleName = currentLine.get().string;
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
	// Class or interface (compiled similarly)
	if (first == "class" || first == "interface")
	{
		scanClass(currentLine, first == "interface", errors).addAttributes(currentAttributes);
		currentAttributes.clear();
		return true;
	}

	// Enum
	if (first == "enum")
	{
		scanEnum(currentLine, errors).addAttributes(currentAttributes);
		currentAttributes.clear();
		return true;
	}

	errors->error(ErrorCode::parseUnexpectedToken, first, "Unexpected '" + first.string + "'");

	return true;
}

ParsedFunction* ds::ParsedFile::scanFunction(TokenLine currentLine, ErrorContext* errors)
{
	bool isAsync = false;

	Token next = currentLine.get();

	while (next != "fn")
	{
		if (next == "async")
		{
			isAsync = true;
			next = currentLine.get();
		}
		else
		{
			return nullptr;
		}
	}

	ParsedFunction& fn = this->functions.emplace_back();

	fn.scanDeclaration(currentLine, stream, this, errors);
	fn.isAsync = isAsync;

	return &fn;
}

ParsedClass& ds::ParsedFile::scanClass(TokenLine currentLine, bool isInterface, ErrorContext* errors)
{
	ParsedClass& newClass = this->classes.emplace_back(this);
	newClass.name = currentLine.get();
	newClass.name.checkIsName(errors);
	newClass.isInterface = isInterface;

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
				errors->error(ErrorCode::parseUnexpectedToken, currentLine.previous(),
					"Expected a ',', got: '" + currentLine.previous().string + "'");
			}
		}
	}
	else
	{
		auto& found = currentLine.get();

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
	// TODO: Complete
	ParsedEnum& newEnum = this->enums.emplace_back();
	newEnum.name = currentLine.get();
	newEnum.name.checkIsName(errors);

	if (currentLine.expect("{", errors))
	{
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
		if (fn.isLambda)
		{
			continue;
		}

#ifdef WITH_LANGUAGE_SERVICE
		if (scanInfo)
		{
			scanInfo->functions.push_back(ScannedFunction(&fn, fn.name, ScannedFunction::Kind::functionDefinition));
		}
#endif
		fn.registerFunction(context);
		fn.compile(context, this, &context->errors);
	}
}

Function* ParsedFile::getMethod(TokenLine& from, ErrorContext* errors)
{
	auto initialPos = from.savePosition();
	auto& name = from.get();
	auto m = getMethod(name, from, errors);
	if (m)
	{
		return m;
	}
	from.loadPosition(initialPos);
	return nullptr;
}

Function* ds::ParsedFile::getMethod(Token name, TokenLine& from, ErrorContext* errors)
{
	auto pos = from.savePosition();

	Function* found = this->fileModule->getMethod(name, from, errors);
	if (found)
		return found;

	for (auto& i : this->usings)
	{
		if (!i.second)
			continue;
		from.loadPosition(pos);
		found = i.second->getMethod(name, from, errors);
		if (found)
			return found;
	}
	from.loadPosition(pos);
	return nullptr;
}

Type* ParsedFile::getType(TokenLine& from, ErrorContext* errors)
{
	auto initialPos = from.savePosition();
	auto& name = from.get();
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
	auto& name = from.get();
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
	auto& name = from.get();
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
