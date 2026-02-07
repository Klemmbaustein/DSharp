#include <ds/parser/typeRegistry.hpp>
#include <ds/parser/types/type.hpp>
#include <ds/parser/types/arrayType.hpp>

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
	for (auto& [_, i] : this->arrayTypes)
	{
		if (i.owner == this)
		{
			delete i.type;
		}
	}
	for (auto& [_, t] : this->genericTypes)
	{
		for (auto& i : t)
		{
			if (i.owner == this)
			{
				delete i.type;
			}
		}
	}
}

std::vector<Type*> ds::TypeRegistry::getAllTypes()
{
	std::vector<Type*> all;

	for (auto& i : this->types)
	{
		all.push_back(i.type);
	}
	for (auto& [_, i] : this->arrayTypes)
	{
		all.push_back(i.type);
	}
	for (auto& [_, t] : this->genericTypes)
	{
		for (auto& i : t)
		{
			all.push_back(i.type);
		}
	}

	return all;
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

ArrayType* ds::TypeRegistry::getArray(Type* base)
{
	Entry& instance = arrayTypes[base];
	if (!instance.owner)
	{
		instance.owner = this;
		instance.type = new ArrayType(base, this);
	}
	return static_cast<ArrayType*>(instance.type);
}