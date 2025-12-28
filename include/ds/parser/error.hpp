#pragma once
#include "tokens.hpp"
#include <functional>

namespace ds
{
	enum class ErrorCode
	{
		/// E1000 internal compiler error.
		internalError = 1000,
		/// E2000 unexpected end of line while tokenizing.
		tokenUnexpectedEof = 2000,
		/// E2001 unexpected ::
		tokenUnexpectedDoubleColon = 2001,
		/// E2002 Expected a closing quote.
		tokenExpectedClosingQuote = 2002,
		/// E2002 Expected a closing quote.
		tokenUnexpectedEndOfLine = 2003,
		/// E3000 expected a {
		parseExpectedOpenBracket = 3000,
		/// E3001 unexpected end of line while parsing.
		parseUnexpectedEof = 3001,
		/// E3002 this token was not expected here.
		parseUnexpectedToken = 3002,
		/// E3003 this method, variable or type is unknown.
		parseUnknownSymbol = 3003,
		/// E3004 could not determine type of expression.
		parseUnknownExpressionType = 3004,
		/// E3005 unexpected or wrong type.
		parseInvalidType = 3005,
		/// E3006 unknown module.
		parseUnknownModule = 3006,
		/// E3007 unknown member.
		parseUnknownMember = 3007,
		/// E3008 this value cannot be written to.
		parseReadOnlyValue = 3008,
		/// E3009 variable declared with "var" must have an initializer.
		parseVarMustHaveInitializer = 3009,
		/// E3010 This string format is not valid.
		parseInvalidFormat = 3010,
		/// E3011 Expected a name here.
		parseExpectedName = 3011,
		/// E3012 Override does not override an existing function or there's a type mismatch.
		parseInvalidOverride = 3012,
		/// E3013 Variable redefinition
		parseVariableRedefinition = 3013,
		/// E3014 No matching constructor
		parseNoMatchingConstructor = 3014,
		/// E3014 Await can only be used in async functions
		parseInvalidAwait = 3015,
		/// E4000 NoDiscard function return value discarded.
		returnValueDiscarded = 4000,
	};

	struct ErrorContext
	{
		void reset()
		{
			hasError = false;
		}
		void error(ErrorCode code, const Token& at, std::string description);

		std::function<void(ErrorCode code, std::string file,
			const Token& at, std::string description)> errorCallback;

		std::function<void(std::string)> writeError;

		std::string currentFile;

		bool isOk() const
		{
			return !hasError;
		}

	private:
		bool hasError = false;
	};
} // namespace ds