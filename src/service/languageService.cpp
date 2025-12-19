#ifdef WITH_LANGUAGE_SERVICE
#include <ds/service/languageService.hpp>
#include <algorithm>

using namespace ds;

ds::LanguageService::LanguageService(LanguageContext* context)
{
	this->parser = new ParseContext(context);
	this->parser->service = this;
}

void ds::LanguageService::addString(const std::string& content, std::string name)
{
	this->parser->addString(content, name);
}

void ds::LanguageService::updateFile(const std::string& str, std::string fileName)
{
	this->parser->updateFile(str, fileName);
}

void ds::LanguageService::commitChanges()
{
	this->parser->resetModules();
	this->parser->errors.reset();
	this->parser->compile();
}

std::vector<AutoCompleteResult> ds::LanguageService::completeAt(ScannedFile* f, size_t character, size_t line,
	CompletionType type)
{
	if (type == CompletionType::variables)
	{
		return completeScopeContents(f, character, line);
	}
	ScannedFunction* foundFunction = nullptr;
	ScannedVariable* found = nullptr;
	size_t nearestLine = 0;

	auto listScopeContents = [&]() -> std::vector<AutoCompleteResult> {
		if (type == CompletionType::member)
		{
			return {};
		}
		return completeScopeContents(f, character, line);
	};

	for (auto& i : f->variables)
	{
		if (i.at.position.line == line && i.at.position.endPos <= character &&
			i.at.position.startPos <= character && nearestLine < i.at.position.endPos)
		{
			nearestLine = i.at.position.endPos;
			found = &i;
		}
	}

	for (auto& i : f->functions)
	{
		if (i.at.position.line == line && i.argEnd.endPos <= character &&
			i.at.position.startPos <= character && nearestLine < i.argEnd.endPos)
		{
			nearestLine = i.argEnd.endPos;
			found = nullptr;
			foundFunction = &i;
		}
	}

	TypeId typeToList = 0;

	if (found)
	{
		typeToList = found->typeId;
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

	std::vector<AutoCompleteResult> result;
	for (auto& i : t->second.members)
	{
		result.push_back(AutoCompleteResult{
			.name = i.name });
	}

	for (auto& i : t->second.methods)
	{
		result.push_back(AutoCompleteResult{
			.name = i.shortName.empty() ? i.name : i.shortName });
	}

	std::sort(result.begin(), result.end(), [](const AutoCompleteResult& a, const AutoCompleteResult& b) {
		return a.name < b.name;
	});

	return result;
}
std::vector<AutoCompleteResult> ds::LanguageService::completeScopeContents(ScannedFile* f, size_t character, size_t line)
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

	if (innermostScope)
	{
		for (auto& i : innermostScope->localVariables)
		{
			result.push_back(AutoCompleteResult{ .name = i });
		}
	}

	return result;
}
#endif
