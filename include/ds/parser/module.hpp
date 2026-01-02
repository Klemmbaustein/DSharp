#pragma once
#include <map>
#include <string>
#include "function.hpp"
#include "attribute.hpp"
#include "types/enumType.hpp"

namespace ds
{
	struct ParsedFile;
	struct ParseContext;

	struct Module
	{
		std::string name;

		std::map<std::string, Type*> moduleTypes;
		std::map<std::string, Function*> moduleFunctions;
		std::map<std::string, Attribute*> moduleAttributes;
		std::map<std::string, EnumType*> moduleEnums;

		std::map<std::string, Module*> submodules;

		Function* getMethod(Token name, TokenLine& from, ErrorContext* errors);
		Type* getType(Token name, TokenLine& from, ErrorContext* errors, ParsedFile* file, ParseContext* context);
		Attribute* getAttribute(Token name, TokenLine& from, ParsedFile* file, ParseContext* context);
		std::pair<EnumType*, std::string> getEnum(std::string name, TokenLine& from);

		Module* checkForSubmodule(std::string& name);
	};
} // namespace ds