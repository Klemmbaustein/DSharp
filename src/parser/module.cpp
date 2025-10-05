#include <ds/parser/module.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/types/functionType.hpp>
#include <ds/parser/types/arrayType.hpp>
#include <ds/service/languageService.hpp>
#include <ds/parser/parser.hpp>
using namespace ds;

Function* ds::Module::getMethod(std::string name)
{
	auto foundFunction = this->moduleFunctions.find(name);

	if (foundFunction != this->moduleFunctions.end())
	{
		return foundFunction->second;
	}

	Module* foundModule = checkForSubmodule(name);

	if (foundModule)
	{
		return foundModule->getMethod(name);
	}

	return nullptr;
}

Type* ds::Module::getType(Token name, TokenLine& from, ErrorContext* errors,
	ParsedFile* file, ParseContext* context)
{
	Type* found = nullptr;
	if (name == "fn")
	{
		found = FunctionType::compileType(name, from, errors, file);
	}
	else
	{
		auto foundType = this->moduleTypes.find(name.string);

		if (foundType == this->moduleTypes.end())
		{
			Module* foundModule = checkForSubmodule(name.string);

			if (foundModule)
			{
				return foundModule->getType(name, from, errors, file, context);
			}
			return nullptr;
		}

#ifdef WITH_LANGUAGE_SERVICE
		if (context->service)
		{
			context->service->files[file->name].types.push_back(name);
		}
#endif

		found = foundType->second;
	}

	if (from.peek() == "?")
	{
		auto nullableType = dynamic_cast<ClassType*>(found);
		from.get();
		if (nullableType)
		{
			found = nullableType->nullable;
		}
	}

	// array
	if (from.peek() == "[")
	{
		from.get();
		from.get();
		found = ArrayType::getInstance(found);
	}

	return found;
}

Attribute* ds::Module::getAttribute(Token name, TokenLine& from, ParsedFile* file, ParseContext* context)
{
	auto foundAttribute = this->moduleAttributes.find(name.string);

	if (foundAttribute != this->moduleAttributes.end())
	{

#ifdef WITH_LANGUAGE_SERVICE
		if (context->service)
		{
			context->service->files[file->name].types.push_back(name);
		}
#endif
		return foundAttribute->second;
	}
	Module* foundModule = checkForSubmodule(name.string);

	if (foundModule)
	{
		return foundModule->getAttribute(name, from, file, context);
	}

	return nullptr;
}

std::pair<EnumType*, std::string> ds::Module::getEnum(std::string name, TokenLine& from)
{
	size_t lastColon = name.find_last_of(":");
	auto nameSubstr = name.substr(0, lastColon - 1);

	auto foundEnum = this->moduleEnums.find(nameSubstr);

	if (foundEnum != this->moduleEnums.end())
	{
		return { foundEnum->second, name.substr(lastColon + 1) };
	}
	Module* foundModule = checkForSubmodule(name);

	if (foundModule)
	{
		return foundModule->getEnum(name, from);
	}

	return { nullptr, "" };
}

Module* ds::Module::checkForSubmodule(std::string& name)
{
	for (auto& [modName, module] : this->submodules)
	{
		std::string moduleName = this->name.empty() ? modName : modName.substr(this->name.size() + 2);

		moduleName.append("::");

		if (name.substr(0, moduleName.size()) == moduleName)
		{
			name = name.substr(moduleName.size());
			return module;
		}
	}
	return nullptr;
}
