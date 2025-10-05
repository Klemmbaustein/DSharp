#pragma once
#include "attribute.hpp"
#include "types/enumType.hpp"
#include <map>

namespace ds
{
	struct ParsedFile;
	struct ParseContext;
	struct ParsedClass;

	class ParsedEnum : public Attributable
	{
	public:
		std::map<Token, int32_t> values;
		Token name;
		TokenStream scope;

		EnumType* thisType = nullptr;

		void registerType(ParseContext* context, ParsedFile* file);
	};
}