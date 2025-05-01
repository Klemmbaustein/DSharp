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
		"This value is discarded, but it isn't marked as [Discard].\n"
			"If the return value of this function is supposed to be discardable, add the system::Discard attribute in front of the function.");
}

void lang::ExpressionResult::compileToType(Token at, Type* target, ErrorContext* errors)
{
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
