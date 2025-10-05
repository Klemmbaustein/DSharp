#pragma once
#include <ds/native/nativeModule.hpp>

namespace ds::modules::system
{
	/**
	 * @ingroup stdlib
	 * @brief
	 * Module for input/output operations.
	 */
	namespace io
	{
		/**
		 * @brief
		 * Creates the system::io module.
		 *
		 * @see ds::modules::createStandardLibrary
		 */
		NativeModule createModule();

		class File
		{
		public:
			FILE* handle;
		};
	}
}