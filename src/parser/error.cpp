#include <ds/parser/error.hpp>
#include <format>
#include <iostream>

void ds::ErrorContext::error(ErrorCode code, const Token& at, std::string description)
{
	this->hasError = true;

	if (this->errorCallback)
	{
		this->errorCallback(code, currentFile, at, description);
		return;
	}

#if HAS_CPP_FORMAT

	if (!this->writeError)
	{
		this->writeError = [](std::string str) {
			std::puts(str.c_str());
		};
	}

	this->writeError(std::format("{}: line {}, {}: E{:04} - {}",
		this->currentFile, at.position.line + 1,
		at.position.startPos + 1, int(code), description));
#else
	std::printf("%s: line %li, %li: E%i - %s\n", this->currentFile.c_str(), at.position.line + 1, at.position.startPos + 1, int(code), description.c_str());
#endif
}
