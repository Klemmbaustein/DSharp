#if WITH_WINAPI
#pragma once
#include <native/nativeModule.hpp>

namespace lang::modules::system
{
	/**
	 * @ingroup stdlib
	 * @brief
	 * Windows API bindings
	 */
	namespace win32
	{
		/**
		 * @brief
		 * Creates the system::win32 module.
		 *
		 * @see lang::modules::createStandardLibrary
		 */
		NativeModule createModule();
	} // namespace win32
} // namespace lang::modules::system
#endif