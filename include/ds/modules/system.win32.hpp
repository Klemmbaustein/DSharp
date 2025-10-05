#if WITH_WINAPI
#pragma once
#include <ds/native/nativeModule.hpp>

namespace ds::modules::system
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
		 * @see ds::modules::createStandardLibrary
		 */
		NativeModule createModule();
	} // namespace win32
} // namespace ds::modules::system
#endif