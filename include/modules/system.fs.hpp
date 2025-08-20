#if MODULE_FS
#pragma once
#include <native/nativeModule.hpp>

namespace lang::modules::system
{
	/**
	 * @ingroup stdlib
	 * @brief
	 * Module containing filesystem functions.
	 */
	namespace fs
	{
		/**
		 * @brief
		 * Creates the system::fs module.
		 *
		 * @see lang::modules::createStandardLibrary
		 */
		NativeModule createModule();

		class FilePath
		{
		public:
			RuntimeStr pathString;
		};

		RuntimeClass* createPath();
	} // namespace fs
} // namespace lang::modules::system
#endif