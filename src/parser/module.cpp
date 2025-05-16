#include <parser/module.hpp>
using namespace lang;

Function* lang::Module::getMethod(std::string name)
{
	auto foundFunction = this->moduleFunctions.find(name);

	if (foundFunction != this->moduleFunctions.end())
	{
		return foundFunction->second;
	}

	return nullptr;
}

Type* lang::Module::getType(TokenLine& from)
{
	auto name = from.get();

	auto foundType = this->moduleTypes.find(name.string);

	if (foundType == this->moduleTypes.end())
	{
		return nullptr;
	}

	// array
	if (from.peek() == "[")
	{
		abort();
	}

	return foundType->second;
}

Attribute* lang::Module::getAttribute(TokenLine& from)
{
	auto name = from.get();

	auto foundFunction = this->moduleAttributes.find(name.string);

	if (foundFunction != this->moduleAttributes.end())
	{
		return foundFunction->second;
	}

	return nullptr;
}
