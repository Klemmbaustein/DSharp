#include <ds/parser/attribute.hpp>
#include <ds/parser/error.hpp>
#include <ds/parser/parser.hpp>

using namespace ds;

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
		auto line = TokenLine();
		line.lineTokens = &i.attributeTokens;
		i.attribute = file->getAttribute(line);
		if (!i.attribute)
		{
			errors->error(ErrorCode::parseUnknownSymbol, line.lineTokens->at(0),
				"Unknown attribute '" + line.lineTokens->at(0).string + "'");
			continue;
		}

		while (!line.empty())
		{
			Token name = line.get();

			if (i.attribute->attributeParameters.find(name.string) == i.attribute->attributeParameters.end())
			{
				errors->error(ErrorCode::parseUnknownSymbol, name,
					"Unknown attribute parameter '" + name.string + "'");
			}

			if (line.expect("=", errors))
			{
				break;
			}

			Token value = line.get();

			i.parameters.insert({ name, value });
		}
	}
}

std::vector<std::string> ds::Attributable::AttribInfo::parametersToString() const
{
	std::vector<std::string> result;

	for (std::pair<Token, Token> i : this->parameters)
	{
		i.second.unquoteString();
		result.push_back(i.first.string + "=" + i.second.string);
	}

	return result;
}

void ds::Attributable::clearAttributes()
{
	this->attributes.clear();
}

TypeId ds::Attribute::getType() const
{
	return typeIdFromName(this->moduleName + "::" + this->name);
}