#include <parser/expression.hpp>
#include <parser/error.hpp>
#include <parser/type.hpp>

void lang::ExpressionResult::discard(Token at, ErrorContext* errors)
{
	if (this->discardable || !this->type)
	{
		return;
	}
	errors->error(ErrorCode::returnValueDiscarded, at,
		"This value is discarded, but isn't discardable.\n"
		"If this is a function and the return value is supposed to be discardable, add the Discard attribute.");
}

void lang::ExpressionResult::compileToType(Token at, Type* target, ErrorContext* errors)
{
	if (!type)
	{
		return;
	}

	if (this->type->sameAs(target))
	{
		return;
	}

	auto cast = target->compileCast(*this);
	if (!cast.valid)
	{
		errors->error(ErrorCode::parseInvalidType, at,
			"Type mismatch. Expected " + target->name + ", got " + type->name + " and no cast is possible.");
		return;
	}

	*this = cast;
}
