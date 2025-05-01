#include <parser/stringUtils.hpp>

bool lang::isNumber(std::string text, bool isFloatingPoint)
{
	if (text.empty())
	{
		return false;
	}
	int dot_counter = 0;
	size_t length = 1;
	for (char character = text[1]; character != '\0'; ++length, character = text[length])
	{
		bool const is_valid_character = (character >= '0' && character <= '9') || (character == '.' && ++dot_counter == 1);

		if (is_valid_character == false)
		{
			return false;
		}
	}

	char const first_character = text[0];
	bool is_character_sign = (first_character == '-' || first_character == '+');
	// Verifications for first left character
	if ((is_character_sign || first_character == '.') && length == 1)
	{
		return false;
	}
	if (length == 2 && is_character_sign && text[1] == '.')
	{
		return false;
	}

	if (dot_counter > 0 != isFloatingPoint)
	{
		return false;
	}

	return (is_character_sign || first_character == '.') || (first_character >= '0' && first_character <= '9');
}
