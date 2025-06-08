#pragma once
#include <string>

namespace lang
{
	enum class Operator
	{
		/// a + b operator
		add,
		/// a - b operator
		subtract,
		/// a * b operator
		multiply,
		/// a / b operator
		divide,
		/// a % b operator
		modulo,
		/// a == b operator
		equals,
		/// a != b operator
		notEquals,
		/// a > b operator
		greater,
		/// a < b operator
		less,
		/// a >= b operator
		greaterEquals,
		/// a <= b operator
		lessEquals,
		/// a && b operator
		logicalAnd,
		/// a.member operator
		member,
		/// Unknown operator
		unknown,
	};

	enum class CompoundOperator
	{
		add,
		subtract,
		multiply,
		divide,
		modulo,
		unknown,
	};

	/**
	 * @brief
	 * Converts a string to an operator enum value.
	 *
	 * @see Operator
	 */
	Operator stringToOperator(std::string opString);

	CompoundOperator stringToCompoundOperator(std::string opString);

	bool operatorHasPriority(Operator op, Operator compareTo);
} // namespace lang