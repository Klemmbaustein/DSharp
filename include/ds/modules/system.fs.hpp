#if MODULE_FS
#pragma once
#include <ds/native/nativeModule.hpp>

namespace ds::modules::system
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
		 * @see ds::modules::createStandardLibrary
		 */
		NativeModule createModule();

		class FilePath
		{
		public:
			RuntimeStr pathString;
		};

		RuntimeClass* createPath();
	} // namespace fs
} // namespace ds::modules::system
#endif