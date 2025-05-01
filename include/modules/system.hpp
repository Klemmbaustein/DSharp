#pragma once
#include <native/nativeModule.hpp>
#include <parser/attribute.hpp>

namespace lang::modules
{
	/**
	 * @ingroup stdlib
	 * @brief
	 * Base standard library module.
	 *
	 * It contains the basic standard library functionalities.
	 */
	namespace system
	{
		/**
		 * @ingroup stdlib
		 *
		 * @brief
		 * Entry point attribute.
		 *
		 * Marks the function it's applied to as the program entry point.
		 * The program entry point is the default function that's called when calling InterpretContext::run()
		 * Only one EntryPoint attribute can be used in a program.
		 *
		 * @see Attribute
		 * @see InterpretContext
		 */
		class EntryPointAttribute : public Attribute
		{
		public:
			EntryPointAttribute()
			{
				this->name = "EntryPoint";
			}
		};

		/**
		 * @ingroup stdlib
		 *
		 * @brief
		 * No discard.
		 *
		 * Marks the return value of the function as Discard. If this attribute isn't appled to a function,
		 * the compiler will emit an error if the return value of this function is ignored.
		 *
		 * @see Attribute
		 * @see Function
		 */
		class DiscardAttribute : public Attribute
		{
		public:
			DiscardAttribute()
			{
				this->name = "Discard";
			}
		};

		/**
		 * @brief
		 * Creates the system module.
		 *
		 * @see lang::modules::createStandardLibrary
		 */
		NativeModule createModule();
	} // namespace system
} // namespace lang::modules