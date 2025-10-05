#pragma once
#include <ds/language.hpp>

/**
 * @defgroup stdlib Standard library
 *
 * @brief
 * Provides common functions and types useful for all programs.
 *
 * The standard library is implemented as many native modules containing parts of it.
 *
 * ## List of modules:
 * - system - @ref ds::modules::system
 * - system.io - @ref ds::modules::system::io
 */

/**
* @brief
* Modules namespace
*
* Contains modules provided by the language library.
*/
namespace ds::modules
{
	/**
	 * @ingroup stdlib
	 *
	 * @brief
	 * Registers the standard library to the given language context.
	 */
	void registerStandardLibrary(LanguageContext* context);
} // namespace ds::modules