#include <parser/error.hpp>
#include <print>
#include <iostream>

void lang::ErrorContext::error(ErrorCode code, const Token& at, std::string description)
{
	this->hasError = true;

#if HAS_CPP_FORMAT
	std::printf("%s: line %li, %li: E%i - %s\n", this->currentFile.c_str(), at.position.line + 1, at.position.startPos + 1, int(code), description.c_str());
#else
	 std::println("{}: line {}, {}: E{:04} - {}", this->currentFile, at.position.line + 1, at.position.startPos + 1, int(code), description);
#endif

}
