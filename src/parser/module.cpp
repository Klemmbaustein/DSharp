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

Type* lang::Module::getType(TokenLine& from)
{
	auto name = from.get();

	auto foundType = this->moduleTypes.find(name.string);

	if (foundType == this->moduleTypes.end())
	{
		Module* foundModule = checkForSubmodule(name.string);

		if (foundModule)
		{
			from.position -= 1;
			from.lineTokens->at(from.position) = name;
			return foundModule->getType(from);
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

Attribute* lang::Module::getAttribute(TokenLine& from)
{
	Token name = from.get();

	auto foundFunction = this->moduleAttributes.find(name.string);

	if (foundFunction != this->moduleAttributes.end())
	{
		return foundFunction->second;
	}
	Module* foundModule = checkForSubmodule(name.string);

	if (foundModule)
	{
		from.position -= 1;
		from.lineTokens->at(from.position) = name;
		return foundModule->getAttribute(from);
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
