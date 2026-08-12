#pragma once
#include "attribute.hpp"
#include "parseFunction.hpp"
#include <list>
#include "parseEnum.hpp"

namespace ds
{
	struct ParsedFile : Attributable
	{
		/// The tokens of this file.
		TokenStream stream;
		/// All functions found in this file.
		std::list<ParsedFunction> functions;
		/// Temporary file functions, such as lambdas etc.
		std::list<ParsedFunction> temporaryFunctions;
		/// All classes found in this file.
		std::list<ParsedClass> classes;
		/// All enums found in this file. (WIP)
		std::list<ParsedEnum> enums;
		/// All the modules that are included using `using xy` in this file.
		std::map<Token, Module*> usings;
		/// The module this file belongs to.
		Module* fileModule = nullptr;
		/// The parse context that created this file.
		ParseContext* context = nullptr;

		/// The name of this file.
		std::string name;
		/// Usually the same as name, change this for code generated from another file.
		std::string displayName;
		/// The name of the module this file belongs to
		std::string moduleName;

		/// Parses a method from this file, using the modules in `usings`.
		Function* getMethod(TokenLine& from, ErrorContext* errors);
		/// Parses a method from this file, using the modules in `usings`.
		Function* getMethod(Token name, TokenLine& from, ErrorContext* errors);
		/// Parses a constant from this file, using the modules in `usings`.
		ExpressionResult getConstant(Token name, ErrorContext* errors);
		/// Parses a type from this file, using the modules in `usings`.
		Type* getType(TokenLine& from, ErrorContext* errors);
		/// Parses an attribute from this file, using the modules in `usings`.
		Attribute* getAttribute(TokenLine& from);
		std::pair<EnumType*, std::string> getEnum(TokenLine& from);

		/// Updates this file's using modules, finding modules corresponding to the names to look for.
		void updateUsings();

		/// Scans this file for function declarations, usings, classes etc. without compiling anything yet.
		void scan(ErrorContext* errors);

		/// Compiles everything in this file, turning the scanned functions, classes etc into bytecode instructions.
		void compile(ParseContext* context);

	private:
		bool scanLine(std::vector<AttribInfo>& currentAttributes, ErrorContext* errors);
		ParsedFunction* scanFunction(TokenLine currentLine, ErrorContext* errors);
		ParsedClass& scanClass(TokenLine currentLine, bool isInterface, ErrorContext* errors);
		ParsedEnum& scanEnum(TokenLine currentLine, ErrorContext* errors);
	};
} // namespace ds