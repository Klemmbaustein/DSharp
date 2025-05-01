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
		/// a.member operator
		member,
		/// Unknown operator
		unknown,
	};

	/**
	* @brief
	* Converts a string to an operator enum value.
	*
	* @see Operator
	*/
	Operator stringToOperator(std::string opString);

	bool operatorHasPriority(Operator op, Operator compareTo);
}