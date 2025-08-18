#include <parser/parseEnum.hpp>
#include <parser/parser.hpp>

using namespace lang;

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
