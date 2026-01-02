#include <ds/parser/functionArgument.hpp>
#include <ds/parser/types/type.hpp>

bool ds::FunctionArgument::operator==(const FunctionArgument& other) const
{
	return this->type->sameAs(other.type);
}