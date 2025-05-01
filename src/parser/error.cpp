#include <parser/error.hpp>
#include <print>

void lang::ErrorContext::error(ErrorCode code, const Token& at, std::string description)
{
	this->hasError = true;
	std::println("{}: line {}, {}: E{:04} - {}", this->currentFile, at.position.line + 1, at.position.startPos + 1, int(code), description);
}
