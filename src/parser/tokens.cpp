#include <parser/tokens.hpp>
#include <parser/error.hpp>
#include <parser/stringUtils.hpp>
#include <set>

namespace lang
{
	static std::set<std::string> specialWords = {
		"==",
		">=",
		"<=",
		"->",
		"::",
	};

	static std::set<char> specialChars = {
		'=',
		'>',
		'<',
		'+',
		'-',
		'*',
		'/',
		':',
		'(',
		')',
		'[',
		']',
		'{',
		'}',
		','
	};

	static std::set<char> whitespace = {
		' ',
		'\t',
		'\r',
	};

} // namespace lang

using namespace lang;

void Token::addChar(char c)
{
	this->string.push_back(c);
	this->position.endPos++;
}

void lang::Token::addStr(std::string str)
{
	this->string.append(str);
	this->position.endPos += uint32_t(str.size());
}

void lang::Token::merge(const Token& other)
{
	this->string += other.string;
	this->position.endPos = other.position.endPos;
}

void lang::TokenStream::fromFile(std::string path, ErrorContext* errors)
{
	std::ifstream file = std::ifstream(path);
	fromStream(file, path, errors);
	file.close();
}

void lang::TokenStream::fromStream(std::istream& stream, std::string name, ErrorContext* errors)
{
	Token currentWord = newToken();

	currentLine = &this->lineTokens.emplace_back();

	while (true)
	{
		char newChar = getNextChar(stream);

		if (stream.eof() || newChar == EOF)
			break;

		if (newChar == '\n')
		{
			addToken(currentWord);
			this->character = 0;
			this->line++;

			if (tryReadChar(stream, '{'))
			{
				auto charToken = newToken();
				charToken.addChar('{');
				addToken(charToken);
			}

			if (!currentLine->empty())
			{
				currentLine = &this->lineTokens.emplace_back();
			}
			currentWord = newToken();
			continue;
		}

		if (newChar == '"')
		{
			currentWord.addChar(newChar);
			do
			{
				newChar = getNextChar(stream);
				currentWord.addChar(newChar);
			} while (newChar != '"');
			addToken(currentWord);
			currentWord = newToken();
			continue;
		}

		bool foundSpecial = false;
		for (auto& i : specialWords)
		{
			if (i[0] == newChar && tryReadWord(stream, i, 1))
			{
				addToken(currentWord);
				currentWord = newToken();
				currentWord.addStr(i);
				currentWord.position.startPos -= uint32_t(i.size());
				currentWord.position.endPos -= uint32_t(i.size());
				addToken(currentWord);

				currentWord = newToken();
				foundSpecial = true;
				break;
			}
		}
		if (foundSpecial)
			continue;


		if (specialChars.contains(newChar) || (newChar == '.' && !isNumber(currentWord.string, false)))
		{
			addToken(currentWord);
			auto charToken = newToken();
			charToken.addChar(newChar);
			currentLine->push_back(charToken);
			currentWord = newToken();
			continue;
		}

		if (whitespace.contains(newChar))
		{
			addToken(currentWord);
			currentWord = newToken();
			continue;
		}
		currentWord.addChar(newChar);
	}
	addToken(currentWord);

	if (currentLine->empty())
	{
		currentLine = nullptr;
		this->lineTokens.pop_back();
	}
}

bool lang::TokenStream::tryReadWord(std::istream& stream, std::string word, size_t beginAt)
{
	StreamPosition startPos = StreamPosition(this, stream);
	readWhitespace(stream);
	for (size_t i = beginAt; i < word.size(); i++)
	{
		if (!tryReadChar(stream, word[i]))
		{
			startPos.apply(this, stream);
			return false;
		}
	}
	return true;
}

bool lang::TokenStream::tryReadChar(std::istream& stream, char c)
{
	StreamPosition startPos = StreamPosition(this, stream);
	while (true)
	{
		int newChar = stream.get();

		if (newChar == EOF)
			break;

		if (whitespace.contains(newChar) || newChar == '\n')
		{
			character++;
			continue;
		}
		if (newChar == c)
		{
			character++;
			return true;
		}
		break;
	}
	startPos.apply(this, stream);
	return false;
}

void lang::TokenStream::readWhitespace(std::istream& stream)
{
	while (true)
	{
		int newChar = stream.get();
		if (newChar == EOF)
			break;

		if (whitespace.contains(newChar))
		{
			character++;
			continue;
		}
		break;
	}
	stream.seekg(-1, stream.cur);
}

char lang::TokenStream::getNextChar(std::istream& stream)
{
	char newChar = stream.get();
	character++;
	return char(newChar);
}

TokenLine& lang::TokenLine::postProcessTokens(ErrorContext* errors)
{
	for (auto it = this->lineTokens->begin(); it < this->lineTokens->end(); it++)
	{
		auto next = it + 1;
		while (next != this->lineTokens->end() && next->string == "::")
		{
			std::vector<Token>::iterator beginIterator = it;
			Token& newToken = *it;

			if (newToken.string == "::")
			{
				errors->error(ErrorCode::tokenUnexpectedDoubleColon, newToken, "Unexpected '::'");
				return *this;
			}

			newToken.merge(*++it);

			it++;

			if (it == this->lineTokens->end())
			{
				errors->error(ErrorCode::tokenUnexpectedEof, *next, "Unexpected '::' after '::'");
				return *this;
			}

			newToken.merge(*it);

			this->lineTokens->erase(beginIterator + 1, beginIterator + 3);

			it = beginIterator;
			next = it + 1;
		}
	}
	return *this;
}

void lang::TokenStream::addToken(Token& target)
{
	if (!target.string.empty())
	{
		currentLine->push_back(target);
	}
}

Token lang::TokenLine::peek()
{
	if (this->empty())
		return Token();
	return this->lineTokens->at(this->position);
}

Token lang::TokenLine::get()
{
	if (this->empty())
		return Token();

	return this->lineTokens->at(this->position++);
}
bool lang::TokenLine::empty() const
{
	return !this->lineTokens || this->lineTokens->empty() || this->position >= this->lineTokens->size();
}

std::vector<Token> lang::TokenLine::getInBraces(ErrorContext* errors)
{
	std::vector<Token> result;

	size_t depth = 0;

	do
	{
		auto next = get();

		if (next.empty())
		{
			errors->error(ErrorCode::parseUnexpectedToken, previous(), "Expected a ')'");
			break;
		}

		if (next.string == "(")
		{
			// Skip first (
			if (depth == 0)
			{
				depth++;
				continue;
			}
			depth++;
		}
		else if (next.string == ")")
		{
			if (depth == 0)
			{
				errors->error(ErrorCode::parseUnexpectedToken, previous(), "Unexpected ')'");
				break;
			}
			depth--;
			// Skip last )
			if (depth == 0)
			{
				continue;
			}
		}
		result.push_back(next);

	} while (depth);

	return result;
}

std::vector<Token> lang::TokenLine::getUntil(std::string token, ErrorContext* errors)
{
	std::vector<Token> result;

	size_t depth = 0;
	Token next;

	do
	{
		next = peek();

		if (next.string == "(")
		{
			// Skip first (
			if (depth == 0)
			{
				depth++;
				continue;
			}
			depth++;
		}
		else if (next.string == ")")
		{
			if (depth == 0)
			{
				errors->error(ErrorCode::parseUnexpectedToken, next, "Unexpected ')'");
			}
			depth--;
			// Skip last )
			if (depth == 0)
			{
				continue;
			}
		}

		if (next.string != token)
		{
			get();
		}
		else
		{
			break;
		}

		result.push_back(next);

	} while (!next.empty());

	return result;
}

bool lang::TokenLine::contains(std::string token) const
{
	for (auto& i : *this->lineTokens)
	{
		if (i.string == token)
		{
			return true;
		}
	}
	return false;
}

Token lang::TokenLine::previous()
{
	return this->lineTokens ? this->lineTokens->at(this->position - 1) : Token();
}

size_t lang::TokenLine::savePosition()
{
	return this->position;
}

void lang::TokenLine::loadPosition(size_t oldPos)
{
	this->position = oldPos;
}

TokenLine lang::TokenStream::next(ErrorContext* errors)
{
	if (currentStreamLine == this->lineTokens.size())
	{
		return TokenLine();
	}
	return TokenLine{
		.lineTokens = &this->lineTokens[currentStreamLine++]
	}.postProcessTokens(errors);
}

void lang::TokenStream::addLine(TokenLine ln)
{
	this->lineTokens.push_back(*ln.lineTokens);
}

void lang::TokenStream::getScope(TokenStream& addTo, ErrorContext* errors, size_t beginDepth)
{
	size_t depth = beginDepth;
	while (true)
	{
		auto functionLine = next(errors);

		if (functionLine.empty())
		{
			errors->error(ErrorCode::parseUnexpectedEof, *this->lineTokens.rbegin()->rbegin(),
				"Unexpected end of file in a function body. Did you forget a "
				"'}'?");
			return;
		}

		if (functionLine.contains("{"))
		{
			depth++;
		}
		if (functionLine.contains("}"))
		{
			depth--;
			if (depth == 0)
			{
				break;
			}
		}

		addTo.addLine(functionLine);
	}
}

Token lang::TokenStream::newToken()
{
	return Token{
		.string = "",
		.position = TokenPos(this->character, this->character, this->line),
	};
}

TokenStream::StreamPosition::StreamPosition(TokenStream* from, std::istream& stream)
{
	this->position = stream.tellg();
	this->character = from->character;
	this->line = from->line;
}

void lang::TokenStream::StreamPosition::apply(TokenStream* to, std::istream& stream) const
{
	stream.seekg(this->position);
	to->character = this->character;
	to->line = this->line;
}
