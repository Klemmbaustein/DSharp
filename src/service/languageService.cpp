#ifdef WITH_LANGUAGE_SERVICE
#include <ds/service/languageService.hpp>
#include <algorithm>

using namespace ds;

ds::LanguageService::LanguageService(LanguageContext* context)
{
	this->parser = new ParseContext(context, {});
	this->parser->service = this;
	this->language = context;
}

ds::LanguageService::~LanguageService()
{
	delete this->parser;
}

void ds::LanguageService::addFile(const std::string& content, std::string name)
{
	this->parser->addString(content, name);
	hasChanges = true;
}

void ds::LanguageService::updateFile(const std::string& str, std::string fileName)
{
	this->parser->updateFile(str, fileName);
	hasChanges = true;
}

void ds::LanguageService::removeFile(std::string name)
{
	this->files.erase(name);
	this->parser->files.remove_if([name](ParsedFile& f) {
		return f.name == name;
	});
	hasChanges = true;
}

ParsedClass* ds::LanguageService::addClass(Token className, std::string moduleName,
	ds::TokenStream& body, std::string fileName, std::vector<std::vector<ds::Token>> superClasses)
{
	hasChanges = true;
	return this->parser->addClass(className, moduleName, body, fileName,
		superClasses);
}

ParsedClass* ds::LanguageService::updateClass(Token className, std::string moduleName,
	ds::TokenStream& body, std::string fileName, std::vector<std::vector<ds::Token>> superClasses)
{
	hasChanges = true;
	return this->parser->updateClass(className, moduleName, body, fileName,
		superClasses);
}

void ds::LanguageService::commitChanges()
{
	if (!hasChanges)
	{
		return;
	}
	hasChanges = false;
	this->parser->resetModules(language);
	this->parser->errors.reset();
	for (auto& i : files)
	{
		i.second.functions.clear();
		i.second.variables.clear();
		i.second.types.clear();
		i.second.accessibleEnums.clear();
		i.second.accessibleFunctions.clear();
		i.second.accessibleTypes.clear();
		i.second.scopes.clear();
	}
	this->types.clear();
	this->parser->compile();
}

std::vector<AutoCompleteResult> ds::LanguageService::completeAt(ScannedFile* f, size_t character, size_t line,
	CompletionType type)
{
	ScannedFunction* foundFunction = nullptr;
	ScannedVariable* foundVariable = nullptr;
	size_t nearestLine = 0;

	auto listScopeContents = [&]() -> std::vector<AutoCompleteResult> {
		return completeScopeContents(f, character, line, type);
	};

	for (auto& i : f->variables)
	{
		if (i.at.position.line == line && i.at.position.endPos <= character &&
			i.at.position.startPos <= character && nearestLine < i.at.position.endPos && character < i.at.position.endPos + 2)
		{
			nearestLine = i.at.position.endPos;
			foundVariable = &i;
		}
	}

	for (auto& i : f->functions)
	{
		if (i.at.position.line == line)
		{
			if (i.argEnd.endPos <= character &&
				i.at.position.startPos <= character &&
				nearestLine < i.argEnd.endPos &&
				character < i.argEnd.endPos + 2)
			{
				nearestLine = i.argEnd.endPos;
				foundVariable = nullptr;
				foundFunction = &i;
			}
			if (i.argEnd.endPos >= character &&
				i.at.position.startPos <= character &&
				nearestLine < i.at.position.endPos &&
				character < i.argEnd.endPos + 2)
			{
				nearestLine = i.argEnd.endPos;
				foundVariable = nullptr;
				foundFunction = nullptr;
			}
		}
	}

	TypeId typeToList = 0;

	if (foundVariable)
	{
		typeToList = foundVariable->typeId;
	}
	else if (foundFunction)
	{
		typeToList = foundFunction->returnTypeId;
	}

	if (typeToList == 0)
	{
		return listScopeContents();
	}

	auto t = types.find(typeToList);

	if (t == types.end())
	{
		return listScopeContents();
	}

	return completeType(&t->second, type);
}

std::vector<AutoCompleteResult> ds::LanguageService::completeScopeContents(ScannedFile* f,
	size_t character, size_t line, CompletionType options)
{
	std::vector<AutoCompleteResult> result;

	ds::ScannedScope* innermostScope = nullptr;

	for (auto& fn : f->scopes)
	{
		if (((fn.start.startPos > character && fn.start.line == line) || fn.start.line < line) &&
			((fn.end.endPos < character && fn.start.line == line) || fn.end.line > line))
		{
			if (!innermostScope || fn.start.line > innermostScope->start.line ||
				(fn.start.line == innermostScope->start.line && fn.start.startPos > innermostScope->start.startPos))
			{
				innermostScope = &fn;
			}
		}
	}

	if (innermostScope && includes(options, CompletionType::variable))
	{
		for (auto& i : innermostScope->localVariables)
		{
			if (i.isThis)
			{
				auto t = types.find(i.type);
				if (t != types.end())
				{
					auto found = completeType(&t->second, options);
					for (auto& i : found)
					{
						result.push_back(i);
					}
				}
			}

			result.push_back(AutoCompleteResult{ .name = i.name,
				.type = CompletionType::variable });
		}

		if (includes(options, CompletionType::keyword))
		{
			for (auto& i : this->scopeKeywords)
			{
				result.push_back(AutoCompleteResult{
					.name = i,
					.type = CompletionType::keyword });
			}
		}
	}
	if (!innermostScope)
	{
		if (includes(options, CompletionType::keyword))
		{
			for (auto& i : this->fileKeywords)
			{
				result.push_back(AutoCompleteResult{
					.name = i,
					.type = CompletionType::keyword });
			}
		}
	}

	try
	{
		if (includes(options, CompletionType::type))
		{
			for (auto& [type, module] : f->accessibleTypes)
			{
				result.push_back(AutoCompleteResult{ .name = types.at(type).name,
					.completionModule = module,
					.type = CompletionType::type });
			}
		}
	}
	catch (std::out_of_range& e)
	{

	}
	if (includes(options, CompletionType::enumValue))
	{
		for (auto& [name, module] : f->accessibleEnums)
		{
			result.push_back(AutoCompleteResult{ .name = name.string,
				.completionModule = module,
				.type = CompletionType::enumValue });
		}
	}

	if (includes(options, CompletionType::function))
	{
		for (auto& [name, module] : f->accessibleFunctions)
		{
			result.push_back(AutoCompleteResult{ .name = name.string,
				.completionModule = module,
				.type = CompletionType::function });
		}
	}

	return result;
}
std::vector<AutoCompleteResult> ds::LanguageService::completeType(ScannedType* type, CompletionType options)
{
	std::vector<AutoCompleteResult> result;
	if (includes(options, CompletionType::member))
	{
		for (auto& i : type->members)
		{
			result.push_back(AutoCompleteResult{
				.name = i.name,
				.type = CompletionType::member });
		}
	}

	if (includes(options, CompletionType::method))
	{
		for (auto& i : type->methods)
		{
			result.push_back(AutoCompleteResult{
				.name = i.shortName.empty() ? i.name : i.shortName,
				.type = CompletionType::method });
		}
	}

	std::sort(result.begin(), result.end(), [](const AutoCompleteResult& a, const AutoCompleteResult& b) {
		return a.name < b.name;
	});

	return result;
}
#endif
