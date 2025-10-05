#include <ds/parser/operator.hpp>
#include <map>
using namespace ds;

Operator ds::stringToOperator(std::string opString)
{
	static std::map<std::string, Operator> operatorStrings = {
		{ "+", Operator::add },
		{ "-", Operator::subtract },
		{ "*", Operator::multiply },
		{ "/", Operator::divide },
		{ "%", Operator::modulo },
		{ "==", Operator::equals },
		{ "!=", Operator::notEquals },
		{ ">", Operator::greater },
		{ "<", Operator::less },
		{ ">=", Operator::greaterEquals },
		{ "<=", Operator::lessEquals },
		{ "and", Operator::logicalAnd },
		{ "or", Operator::logicalOr },
		{ ".", Operator::member },
	};

	auto found = operatorStrings.find(opString);

	if (found != operatorStrings.end())
		return found->second;
	{
	}

	return Operator::unknown;
}

CompoundOperator ds::stringToCompoundOperator(std::string opString)
{
	static std::map<std::string, CompoundOperator> operatorStrings = {
		{ "+=", CompoundOperator::add },
		{ "-=", CompoundOperator::subtract },
		{ "*=", CompoundOperator::multiply },
		{ "/=", CompoundOperator::divide },
		{ "%=", CompoundOperator::modulo },
	};

	auto found = operatorStrings.find(opString);

	if (found != operatorStrings.end())
	{
		return found->second;
	}

	return CompoundOperator::unknown;
}

int32_t ds::getOperatorPriority(Operator op)
{
	static std::map<Operator, int32_t> priorities = {
		{ Operator::add, 0 },
		{ Operator::subtract, 0 },
		{ Operator::multiply, 1 },
		{ Operator::divide, 1 },
		{ Operator::modulo, 1 },
		{ Operator::equals, -10 },
		{ Operator::notEquals, -10 },
		{ Operator::greater, -10 },
		{ Operator::less, -10 },
		{ Operator::greaterEquals, -10 },
		{ Operator::lessEquals, -10 },
		{ Operator::logicalAnd, -20 },
		{ Operator::logicalOr, -20 },
		{ Operator::member, 100 },
	};
	return priorities[op];
}
