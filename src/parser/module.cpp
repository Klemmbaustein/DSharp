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

	// array
	if (from.peek() == "[")
	{
		abort();
	}

	return this->moduleTypes[name.string];
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
