#include <parser/module.hpp>
#include <parser/types/arrayType.hpp>
using namespace lang;

Function* lang::Module::getMethod(std::string name)
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

Type* lang::Module::getType(std::string name, TokenLine& from)
{
	auto foundType = this->moduleTypes.find(name);

	if (foundType == this->moduleTypes.end())
	{
		Module* foundModule = checkForSubmodule(name);

		if (foundModule)
		{
			return foundModule->getType(name, from);
		}
		return nullptr;
	}

	if (from.peek() == "?")
	{
		auto nullableType = dynamic_cast<ClassType*>(foundType->second);
		from.get();

		return nullableType->nullable;
	}

	// array
	if (from.peek() == "[")
	{
		from.get();
		from.get();
		return ArrayType::getInstance(foundType->second);
	}

	return foundType->second;
}

Attribute* lang::Module::getAttribute(std::string name, TokenLine& from)
{
	auto foundFunction = this->moduleAttributes.find(name);

	if (foundFunction != this->moduleAttributes.end())
	{
		return foundFunction->second;
	}
	Module* foundModule = checkForSubmodule(name);

	if (foundModule)
	{
		return foundModule->getAttribute(name, from);
	}

	return nullptr;
}

Module* lang::Module::checkForSubmodule(std::string& name)
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
