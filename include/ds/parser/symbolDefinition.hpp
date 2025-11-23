#pragma once
#include <ds/parser/tokens.hpp>

namespace ds
{
	struct ParsedFile;

	struct SymbolDefinition
	{
		ParsedFile* file = nullptr;
		Token at;
	};
} // namespace ds
