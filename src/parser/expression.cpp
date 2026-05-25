#include <ds/parser/expression.hpp>
#include <ds/parser/error.hpp>
#include <ds/parser/types/type.hpp>

void ds::ExpressionResult::discard(Token at, ErrorContext* errors) const
{
	if (this->discardable || !this->type)
	{
		return;
	}
	errors->error(ErrorCode::returnValueDiscarded, at,
		"Expression value ignored. Assign it to the _ operator if this intentional.");
}

void ds::ExpressionResult::compileToType(Token at, Type* target, ParsedScope* with, ErrorContext* errors)
{
	if (!valid)
	{
		return;
	}

	if (!type)
	{
		if (errors)
		{
			errors->error(ErrorCode::parseInvalidType, at,
				"Type mismatch. Expected " + Type::toString(target) + ", got " + Type::toString(type) + " and no cast is possible.");
		}
		this->valid = false;
		return;
	}

	if (this->type->sameAs(target))
	{
		return;
	}

	auto cast = target->compileCast(*this, with);
	if (!cast.valid && errors)
	{
		errors->error(ErrorCode::parseInvalidType, at,
			"Type mismatch. Expected " + Type::toString(target)
			+ ", got " + Type::toString(type) + " and no cast is possible.");
		return;
	}

	cast.type = target;

	*this = cast;
}

ds::BytecodeBuffer ds::ExpressionResult::unrefHere(ParsedScope* with)
{
	BytecodeBuffer result;
	for (auto& i : this->code.instructions)
	{
		i->unrefHere(&result);
	}
	return result;
}
