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
		// Usually the same as name, change this for code generated from another file.
		std::string displayName;
		std::string scopeName;

		void loadAvailableTypes(ParseContext* context);

		Function* getMethod(TokenLine& from, ErrorContext* errors);
		Function* getMethod(Token name, TokenLine& from, ErrorContext* errors);
		Type* getType(TokenLine& from, ErrorContext* errors);
		Attribute* getAttribute(TokenLine& from);
		std::pair<EnumType*, std::string> getEnum(TokenLine& from);

		void updateUsings();

		void scan(ErrorContext* errors);
		bool scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors);
		ParsedFunction* scanFunction(TokenLine currentLine, ErrorContext* errors);
		ParsedClass& scanClass(TokenLine currentLine, bool isInterface, ErrorContext* errors);
		ParsedEnum& scanEnum(TokenLine currentLine, ErrorContext* errors);

		void compile(ParseContext* context);
	};
}