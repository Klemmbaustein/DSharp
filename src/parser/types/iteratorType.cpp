#include <ds/parser/types/iteratorType.hpp>

ds::IteratorType::IteratorType(Type* baseType, TypeRegistry* registry)
{
	this->baseType = baseType;
	this->isInterface = true;
}