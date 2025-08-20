#pragma once
#include <string>
#include <cstdint>

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
		/// a and b operator
		logicalAnd,
		/// a or b operator
		logicalOr,
		/// not a operator
		logicalNot,
		/// *a operator
		dereference,
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

	int32_t getOperatorPriority(Operator op);
} // namespace lang