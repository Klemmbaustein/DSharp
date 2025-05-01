#include <parser/attribute.hpp>

void lang::Attributable::addAttributes(const std::vector<AttribInfo>& newAttributes)
{
	for (auto& i : newAttributes)
	{
		this->attributes.push_back(i);
	}
}
