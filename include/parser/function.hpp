#pragma once
#include "types/type.hpp"
#include "expression.hpp"
#include "attribute.hpp"

namespace lang
{
	struct FunctionArgument
	{
		FunctionArgument(Type* type, Token name)
		{
			this->type = type;
			this->name = name;
		}

		FunctionArgument(Type* type, std::string name)
		{
			this->type = type;
			this->name = Token(name);
		}

		FunctionArgument() = default;

		Type* type = nullptr;
		Token name;

		bool operator==(const FunctionArgument& other) const
		{
			return other.type->sameAs(this->type);
		}
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
		};

		virtual bytecodeOffset getVirtualOffset() const
		{
			return 0;
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

			if (aReturnType && !bReturnType || !aReturnType && bReturnType)
			{
				return false;
			}

			return a->getArguments() == b->getArguments() && (aReturnType->sameAs(bReturnType));
		}

		std::string getSignatureText();
	};
} // namespace lang