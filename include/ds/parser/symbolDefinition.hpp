#pragma once
#include <ds/parser/tokens.hpp>

namespace ds
{
	struct ParsedFile;

	struct SymbolDefinition
	{
		std::string file;
		Token at;
	};
} // namespace ds
