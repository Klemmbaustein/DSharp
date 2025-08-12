#pragma once
#include <native/nativeModule.hpp>

namespace lang::modules::system
{
	/**
	 * @ingroup stdlib
	 * @brief
	 * Module containing error handling functions.
	 */
	namespace err
	{
		/**
		 * @brief
		 * Creates the system::err module.
		 *
		 * @see lang::modules::createStandardLibrary
		 */
		NativeModule createModule();

	} // namespace err
} // namespace lang::modules::system