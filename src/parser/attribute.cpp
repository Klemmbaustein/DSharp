#include <ds/parser/attribute.hpp>
#include <ds/parser/error.hpp>
#include <ds/parser/parser.hpp>

void ds::Attributable::addAttributes(const std::vector<AttribInfo>& newAttributes)
{
	for (auto& i : newAttributes)
	{
		this->attributes.push_back(i);
	}
}

void ds::Attributable::resolveAttributes(ParsedFile* file, ErrorContext* errors)
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