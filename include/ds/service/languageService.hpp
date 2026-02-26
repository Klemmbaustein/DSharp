#ifdef WITH_LANGUAGE_SERVICE
#pragma once
#include <functional>
#include <ds/parser/parser.hpp>
#include "scannedSymbols.hpp"

namespace ds
{
	struct LanguageContext;

	class ScannedFile
	{
	public:
		std::vector<ScannedFunction> functions;
		std::vector<ScannedVariable> variables;
		std::vector<Token> types;
		std::map<TypeId, std::string> accessibleTypes;
		std::map<Token, std::string> accessibleFunctions;
		std::map<Token, std::string> accessibleEnums;

		std::vector<ScannedScope> scopes;
	};

	// clang-format off
	enum class CompletionType
	{
		variable     = 0b0000001,
		member       = 0b0000010,
		function     = 0b0000100,
		method       = 0b0001000,
		type         = 0b0010000,
		keyword      = 0b0100000,
		enumValue    = 0b1000000,
		classMembers = 0b0001010,
		all = 0xff
	};
	// clang-format on

	struct AutoCompleteResult
	{
		std::string name;
		std::string completionModule;
		CompletionType type = CompletionType::variable;
	};

	/**
	 * @brief
	 * A class giving information about parsed scripts.
	 *
	 * Useful for implementing a LSP or having an integrated editor with syntax highlighting, auto complete etc.
	 */
	class LanguageService
	{
	public:
		/**
		 * @brief
		 * Initializes a language service
		 * @param context
		 */
		LanguageService(LanguageContext* context);

		~LanguageService();

		std::vector<std::string> scopeKeywords = {
			"return",
			"if",
			"else",
			"for",
			"continue",
			"while",
			"break",
			"throw",
			"const",
			"var",
			"new",
			"fn",
			"await",
			"not",
			"and",
			"or"
		};
		std::vector<std::string> fileKeywords = {
			"class",
			"interface",
			"module",
			"fn",
			"async",
			"virtual",
			"override"
		};

		/**
		 * @brief
		 * Adds a file to this service's script context.
		 * @param content
		 * The content of the file.
		 * @param name
		 * The name of the file.
		 */
		void addFile(const std::string& content, std::string name);

		/**
		 * @brief
		 * Updates an existing file with the given new content.
		 *
		 * The file must already exist in this context for this to work.
		 *
		 * @param content
		 * The content of the file
		 * @param name
		 * The name of the file to replace
		 */
		void updateFile(const std::string& content, std::string name);

		/**
		 * @brief
		 * Removes a file from this context.
		 * @param name
		 * The name of this file.
		 */
		void removeFile(std::string name);

		/**
		 * @brief
		 * Adds a class body to this service's script context.
		 *
		 * This is meant to be used to embed the scripting language into other languages.
		 *
		 * @param className
		 * The name of the class to add.
		 * @param moduleName
		 * The name of the module this class is in.
		 * @param body
		 * The body of this class, containing method definitions etc.
		 * @param fileName
		 * The name of the file this class is defined in.
		 * @param superClasses
		 * The tokens containing the classes this class derives from.
		 * @return
		 * A pointer to the new parsed class.
		 */
		ParsedClass* addClass(Token className, std::string moduleName,
			ds::TokenStream& body, std::string fileName, std::vector<std::vector<ds::Token>> superClasses);
		ParsedClass* updateClass(Token className, std::string moduleName,
			ds::TokenStream& body, std::string fileName, std::vector<std::vector<ds::Token>> superClasses);

		// TODO: remove classes

		/**
		 * @brief
		 * Commits any changes made by the add/update/remove functions, recompiling the scripts.
		 */
		void commitChanges();

		/**
		 * @brief
		 * Tries to give possible values to autocomplete at a given character position.
		 * @param f
		 * The file that the completion should occur in.
		 * @param character
		 * The character where the completion should occur at.
		 * @param line
		 * The line where the completion should occur at.
		 * @param type
		 * A filter for the possible results of this function.
		 * @return
		 * A list of all possible completions at the given position the service could find.
		 */
		std::vector<AutoCompleteResult> completeAt(ScannedFile* f, size_t character, size_t line,
			CompletionType type);

		ParseContext* parser = nullptr;
		std::map<std::string, ScannedFile> files;
		std::map<TypeId, ScannedType> types;

	private:
		static bool includes(CompletionType a, CompletionType includes)
		{
			return (int(a) & int(includes)) != 0;
		}

		bool hasChanges = false;
		LanguageContext* language = nullptr;

		std::vector<AutoCompleteResult> completeType(ScannedType* type, CompletionType options);

		std::vector<AutoCompleteResult> completeScopeContents(ScannedFile* f, size_t character,
			size_t line, CompletionType options);
	};
} // namespace ds
#endif