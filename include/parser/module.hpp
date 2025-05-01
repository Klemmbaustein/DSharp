#pragma once
#include <map>
#include <string>
#include "function.hpp"
#include "attribute.hpp"

namespace lang
{
	struct Module
	{
		std::string name;

		std::map<std::string, Type*> moduleTypes;
		std::map<std::string, Function*> moduleFunctions;
		std::map<std::string, Attribute*> moduleAttributes;

		std::map<std::string, Module*> submodules;

		Function* getMethod(std::string name);
		Type* getType(TokenLine& from);
		Attribute* getAttribute(TokenLine& from);
	};
} // namespace lang