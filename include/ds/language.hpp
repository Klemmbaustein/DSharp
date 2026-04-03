#pragma once
#include "native/nativeModule.hpp"
#include "parser/parser.hpp"
#include "parser/typeRegistry.hpp"
#include "service/languageService.hpp"
#include "interpreter.hpp"

/**
 * @brief
 * Language namespace.
 *
 * Contains all language functions
 */
namespace ds
{
	/**
	 * @brief
	 * Language context
	 *
	 * Contains information about global language configuration shared between the parser and interpreter.
	 * It is used to create interpreters and parsers.
	 */
	struct LanguageContext
	{
		~LanguageContext()
		{
			delete registry;
		}

		LanguageRuntime* createRuntime();

		/**
		 * @brief
		 * Creates a parser that compiles files and modules into runnable bytecode.
		 *
		 * This should be called after all native modules (standard library, bindings, etc) have been added to the language context.
		 *
		 * @see ParseContext
		 * @see ParseContext::compile()
		 * @see InterpretContext
		 */
		ParseContext* createCompiler(ParserOptions options = {});

#ifdef WITH_LANGUAGE_SERVICE
		LanguageService* startService();
#endif

		/**
		 * @brief
		 * Adds a native module to this language context.
		 *
		 * A native module contains functions, types and attributes to use in the program.
		 *
		 * @param module
		 * A reference to the module.
		 *
		 * @see NativeModule
		 * @see ParseContext
		 * @see InterpretContext
		 */
		void addNativeModule(const NativeModule& module);

		TypeRegistry* registry = new TypeRegistry();

		std::map<std::string, NativeModule*> languageModules;
	};
} // namespace ds