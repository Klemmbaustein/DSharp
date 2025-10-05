#include <ds/parser/expression.hpp>
#include <ds/parser/error.hpp>
#include <ds/parser/types/type.hpp>

void ds::ExpressionResult::discard(Token at, ErrorContext* errors)
{
	if (this->discardable || !this->type)
	{
		return;
	}
	errors->error(ErrorCode::returnValueDiscarded, at,
		"This value is discarded, but isn't discardable.\n"
		"If this is a function and the return value is supposed to be discardable, add the Discard attribute.");
}

void ds::ExpressionResult::compileToType(Token at, Type* target, ParsedScope* with, ErrorContext* errors)
{
	if (!type)
	{
		return;
	}

	if (this->type->sameAs(target))
	{
		return;
	}

	auto cast = target->compileCast(*this, with);
	if (!cast.valid)
	{
		errors->error(ErrorCode::parseInvalidType, at,
			"Type mismatch. Expected " + Type::toString(target) + ", got " + Type::toString(type) + " and no cast is possible.");
		return;
	}

	*this = cast;
}
