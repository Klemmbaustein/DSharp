#include <parser/operator.hpp>
#include <map>
using namespace lang;

Operator lang::stringToOperator(std::string opString)
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
		{ "&&", Operator::logicalAnd },
		{ "||", Operator::modulo },
		{ ".", Operator::member },
	};

	auto found = operatorStrings.find(opString);

	if (found != operatorStrings.end())
	{
		return found->second;
	}

	return Operator::unknown;
}

CompoundOperator lang::stringToCompoundOperator(std::string opString)
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

bool lang::operatorHasPriority(Operator op, Operator compareTo)
{
	static std::map<Operator, int32_t> priorities = {
		{ Operator::add, 0 },
		{ Operator::subtract, 0 },
		{ Operator::multiply, 1 },
		{ Operator::divide, 1 },
		{ Operator::modulo, 1 },
		{ Operator::member, 100 },
	};
	return priorities[op] > priorities[compareTo];
}
