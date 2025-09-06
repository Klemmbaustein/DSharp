#include <parser/attribute.hpp>
#include <parser/error.hpp>
#include <parser/parser.hpp>

void lang::Attributable::addAttributes(const std::vector<AttribInfo>& newAttributes)
{
	for (auto& i : newAttributes)
	{
		this->attributes.push_back(i);
	}
}

void lang::Attributable::resolveAttributes(ParsedFile* file, ErrorContext* errors)
{
	for (auto& i : this->attributes)
	{
		i.attributeTokens[0].checkIsName(errors);

		auto line = TokenLine();
		line.lineTokens = &i.attributeTokens;
		i.attribute = file->getAttribute(line);
		if (!i.attribute)
		{
			errors->error(ErrorCode::parseUnknownSymbol, line.lineTokens->at(0),
				"Unknown attribute '" + line.lineTokens->at(0).string + "'");
		}
	}
}