#pragma once
#include "type.hpp"
#include "expression.hpp"
#include "attribute.hpp"

namespace lang
{
	struct FunctionArgument
	{
		Type* type = nullptr;
		Token name;
	};

	/**
	* @brief
	* A function that can be called in the language.
	*/
	class Function : public Attributable
	{
	public:
		/**
		* @brief
		* Compiles a call for the function, assuming that all arguments are already on the stack.
		*/
		virtual ExpressionResult compileCall() = 0;

		/**
		* @brief
		* Gets all arguments for this function.
		*/
		virtual std::vector<FunctionArgument> getArguments() = 0;

		/**
		* @brief
		* Gets the full name of this function, including the module name.
		*
		* Example: system::io::println for the system::io::println() function
		*/
		virtual std::string getFullName() const = 0;
		virtual bool discardable() const = 0;
	};
} // namespace lang