#pragma once
#include <language.hpp>

/**
 * @defgroup stdlib Standard library
 *
 * @brief
 * Provides common functions and types useful for all programs.
 *
 * The standard library is implemented as many native modules containing parts of it.
 *
 * ## List of modules:
 * - system - @ref lang::modules::system
 * - system.io - @ref lang::modules::system::io
 */

/**
* @brief
* Modules namespace
*
* Contains modules provided by the language library.
*/
namespace lang::modules
{
	/**
	 * @ingroup stdlib
	 *
	 * @brief
	 * Registers the standard library to the given language context.
	 */
	void registerStandardLibrary(LanguageContext* context);
} // namespace lang::modules