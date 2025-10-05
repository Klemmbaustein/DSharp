#include <ds/parser/parseEnum.hpp>
#include <ds/parser/parser.hpp>

using namespace ds;

void ParsedEnum::registerType(ParseContext* context, ParsedFile* file)
{
	thisType = new EnumType();
	thisType->from = name;
	thisType->name = name.string;
	thisType->values = {
		{ Token("x"), 2 },
	};
	file->fileModule->moduleEnums.insert({ name.string, thisType });
}
