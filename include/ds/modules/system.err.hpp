#pragma once
#include <ds/native/nativeModule.hpp>

namespace ds::modules::system
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
		 * @see ds::modules::createStandardLibrary
		 */
		NativeModule createModule();

	} // namespace err
} // namespace ds::modules::system