#include <ds/parser/module.hpp>
#include <ds/parser/parseScope.hpp>
#include <ds/parser/types/functionType.hpp>
#include <ds/parser/types/arrayType.hpp>
#include <ds/service/languageService.hpp>
#include <ds/parser/parser.hpp>
using namespace ds;

ExpressionResult ds::Module::getConstant(Token name, ErrorContext* errors)
{
	auto foundValue = this->moduleConstants.find(name.string);

	if (foundValue != this->moduleConstants.end())
	{
		return foundValue->second;
	}

	Module* foundModule = checkForSubmodule(name.string);

	if (foundModule)
	{
		return foundModule->getConstant(name.string, errors);
	}

	return ExpressionResult();
}

Function* ds::Module::getMethod(Token name, TokenLine& from, ErrorContext* errors)
{
	auto foundFunction = this->moduleFunctions.find(name.string);

	if (foundFunction != this->moduleFunctions.end())
	{
		return foundFunction->second;
	}

	Module* foundModule = checkForSubmodule(name.string);

	if (foundModule)
	{
		return foundModule->getMethod(name.string, from, errors);
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

		if (found->isGeneric)
		{
			auto args = found->getGenericArguments();

			auto types = parseGenericArguments(args, from, errors, file);

			if (checkGenericArguments(args, types, name, errors))
			{
				found = found->instantiateGeneric(types, name, errors, context->registry);
			}
		}
	}

	if (from.peek() == "?")
	{
		auto nullableType = found->asClass();
		if (nullableType)
		{
			from.get();
			found = nullableType->nullable;
		}
	}

	// array
	while (from.peek() == "[")
	{
		from.get();
		from.expect("]", errors);
		found = context->registry->getArray(found);

		if (from.peek() == "?")
		{
			auto nullableType = found->asClass();
			if (nullableType)
			{
				from.get();
				found = nullableType->nullable;
			}
		}
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
		if (module.relativeName.size() > name.size())
		{
			continue;
		}

		if (name.rfind(module.relativeName, 0) == 0)
		{
			name = name.substr(module.relativeName.size());
			return module.module;
		}
	}
	return nullptr;
}

void ds::Module::addModule(const std::string& name, Module* module)
{
	auto& data = submodules[name];

	data.relativeName = this->name.empty() ? name : name.substr(this->name.size() + 2);
	data.relativeName.append("::");
	data.module = module;
}