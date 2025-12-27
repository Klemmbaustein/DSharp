#pragma once
#include <ds/native/nativeModule.hpp>

namespace ds::modules::system
{
	namespace math
	{
		/**
		 * @brief
		 * Creates the system::math module.
		 *
		 * @see ds::modules::createStandardLibrary
		 */
		NativeModule createModule();
	}
} // namespace ds::modules::system