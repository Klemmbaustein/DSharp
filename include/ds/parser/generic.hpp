#pragma once
#include "functionArgument.hpp"
#include "bytecode/compileBytecode.hpp"
#include <ds/parser/typeRegistry.hpp>

namespace ds
{
	struct ParsedScope;
	struct ParsedFile;
	class ClassType;
	class Type;

	struct GenericArgument
	{
		Token name;
		ClassType* baseClassType = nullptr;
	};

	struct GenericParseData
	{
		BytecodeBuffer code;
		std::vector<FunctionArgument> args;
		Type* returnType = nullptr;
	};

	std::vector<Type*> parseGenericArguments(std::vector<GenericArgument> args, TokenLine& line,
		ErrorContext* errors, ParsedFile* with);

	bool checkGenericArguments(std::vector<GenericArgument> args, std::vector<Type*> types,
		Token at, ErrorContext* errors);

	BytecodeBuffer compileGenericArguments(std::vector<Type*> types);

	GenericParseData getGenericFunctionData(ds::Function* fn, TokenLine& line, ErrorContext* errors,
		ParsedScope* with);

	Type* convertGenericType(Type* inType, std::vector<Type*> args, bool isFunction, Token at,
		ErrorContext* errors, TypeRegistry* registry);
} // namespace ds