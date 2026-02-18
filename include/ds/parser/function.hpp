#pragma once
#include "types/type.hpp"
#include "expression.hpp"
#include "attribute.hpp"
#include "symbolDefinition.hpp"
#include "generic.hpp"
#include "functionArgument.hpp"

namespace ds
{
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
		virtual ~Function() = default;

		/**
		 * @brief
		 * Gets all arguments for this function.
		 */
		virtual std::vector<FunctionArgument> getArguments() = 0;
		virtual Type* getReturnType() = 0;

		/**
		 * @brief
		 * Gets the full name of this function, including the module name.
		 *
		 * Example: system::io::println for the system::io::println() function
		 */
		virtual std::string getFullName() const = 0;
		virtual std::string getShortName() const = 0;
		virtual bool discardable() const = 0;

		virtual bool isVirtual() const
		{
			return false;
		}
		virtual BytecodeOffset getVirtualOffset() const
		{
			return 0;
		}

		virtual bool isGeneric() const
		{
			return false;
		}
		virtual std::vector<GenericArgument> getGenericTypes()
		{
			return {};
		}

		virtual std::optional<SymbolDefinition> getDefinition()
		{
			return {};
		}

		virtual BytecodeBuffer compileCallable(ErrorContext* errors, ParsedScope* with, Type* hintType) const;

		static bool signaturesMatch(Function* a, Function* b)
		{
			auto aReturnType = a->getReturnType();
			auto bReturnType = b->getReturnType();

			if (!aReturnType && !bReturnType)
			{
				return a->getArguments() == b->getArguments();
			}

			if ((aReturnType && !bReturnType) || !(aReturnType && bReturnType))
			{
				return false;
			}

			return a->getArguments() == b->getArguments() && (aReturnType->sameAs(bReturnType));
		}

		std::string getSignatureText();
	};
} // namespace ds