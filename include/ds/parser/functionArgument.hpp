#pragma once
#include <ds/parser/tokens.hpp>

namespace ds
{
	class Type;

	struct FunctionArgument
	{
		FunctionArgument(Type* type, Token name)
		{
			this->type = type;
			this->name = name;
		}

		FunctionArgument(Type* type, std::string name)
		{
			this->type = type;
			this->name = Token(name);
		}

		FunctionArgument(Type* type, const char* name)
		{
			this->type = type;
			this->name = Token(name);
		}

		FunctionArgument() = default;

		Type* type = nullptr;
		Token name;

		bool operator==(const FunctionArgument& other) const;
	};
}