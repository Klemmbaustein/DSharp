#pragma once
#include <native/nativeModule.hpp>

namespace lang::modules::system
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
		 * @see lang::modules::createStandardLibrary
		 */
		NativeModule createModule();

		class File
		{
		public:
			FILE* handle;
		};
	}
}