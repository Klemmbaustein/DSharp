#pragma once
#include <map>
#include <string>
#include "function.hpp"
#include "attribute.hpp"
#include "types/enumType.hpp"

namespace lang
{
	struct ParsedFile;

	struct Module
	{
		std::string name;

		std::map<std::string, Type*> moduleTypes;
		std::map<std::string, Function*> moduleFunctions;
		std::map<std::string, Attribute*> moduleAttributes;
		std::map<std::string, EnumType*> moduleEnums;

		std::map<std::string, Module*> submodules;

		Function* getMethod(std::string name);
		Type* getType(std::string name, TokenLine& from, ErrorContext* errors, ParsedFile* file);
		Attribute* getAttribute(std::string name, TokenLine& from);
		std::pair<EnumType*, std::string> getEnum(std::string name, TokenLine& from);

		Module* checkForSubmodule(std::string& name);
	};
} // namespace lang