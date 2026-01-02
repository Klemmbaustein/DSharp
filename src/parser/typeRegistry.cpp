#include <ds/parser/typeRegistry.hpp>
#include <ds/parser/types/type.hpp>

using namespace ds;

ds::TypeRegistry::~TypeRegistry()
{
	for (auto& i : this->types)
	{
		if (i.owner == this)
		{
			delete i.type;
		}
	}
}

bool ds::TypeRegistry::compareTypes(Type* a, Type* b)
{
	if (a == nullptr && b == nullptr)
	{
		return true;
	}
	else if (a == nullptr || b == nullptr)
	{
		return false;
	}
	return a->sameAs(b);
}
