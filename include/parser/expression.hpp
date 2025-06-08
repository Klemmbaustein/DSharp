#pragma once
#include "bytecode/compileBytecode.hpp"
#include <parser/tokens.hpp>
#include <optional>

namespace lang
{
	class Type;
	struct ErrorContext;

	struct ExpressionResult
	{
		bool valid = false;
		bool discardable = false;
		std::optional<BytecodeBuffer> setCode;
		Type* type = nullptr;
		BytecodeBuffer code;

		void discard(Token at, ErrorContext* errors);
		void compileToType(Token at, Type* target, ErrorContext* errors);
	};
}