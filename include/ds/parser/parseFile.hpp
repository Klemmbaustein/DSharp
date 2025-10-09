#pragma once
#include "attribute.hpp"
#include "parseFunction.hpp"
#include <list>
#include "parseEnum.hpp"

namespace ds
{
	struct ParsedFile : Attributable
	{
		~ParsedFile();

		TokenStream stream;
		std::list<ParsedFunction> functions;
		std::list<ParsedClass> classes;
		std::list<ParsedEnum> enums;
		std::map<Token, Module*> usings;
		Module* fileModule = nullptr;
		ParseContext* context = nullptr;

		std::string name;
		std::string scopeName;

		void loadAvailableTypes(ParseContext* context);

		Function* getMethod(std::string name);
		Type* getType(TokenLine& from, ErrorContext* errors);
		Attribute* getAttribute(TokenLine& from);
		std::pair<EnumType*, std::string> getEnum(TokenLine& from);

		void scan(ErrorContext* errors);
		bool scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors);
		ParsedFunction* scanFunction(TokenLine currentLine, ErrorContext* errors);
		ParsedClass& scanClass(TokenLine currentLine, ErrorContext* errors);
		ParsedEnum& scanEnum(TokenLine currentLine, ErrorContext* errors);

		void compile(ParseContext* context);
	};
}