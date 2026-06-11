#include <ds/parser/tokens.hpp>
#include <ds/parser/error.hpp>
#include <ds/parser/stringUtils.hpp>
#include <set>
#include <sstream>

namespace ds
{
	static std::set<std::string> specialWords = {
		"==",
		">=",
		"<=",
		"!=",
		"->",
		"::",
		"+=",
		"-=",
		"*=",
		"/=",
		"%=",
	};

	static std::set<char> specialChars = {
		'=',
		'>',
		'<',
		'+',
		'-',
		'*',
		'/',
		'!',
		'|',
		'&',
		':',
		'(',
		')',
		'[',
		']',
		'{',
		'}',
		',',
		'%',
		';',
		'?'
	};

	static std::set<char> whitespace = {
		' ',
		'\t',
		'\r',
		'\n',
	};

	static Token EMPTY_TOKEN = Token();

} // namespace ds

using namespace ds;

void Token::addChar(char c)
{
	this->string.push_back(c);
	this->position.endPos++;
}

void ds::Token::addStr(std::string str)
{
	this->string.append(str);
	this->position.endPos += uint32_t(str.size());
}

void ds::Token::merge(const Token& other)
{
	this->string += other.string;
	this->position.endPos = other.position.endPos;
}

bool ds::Token::checkIsName(ErrorContext* errors) const
{
	if (this->empty())
	{
		errors->error(ErrorCode::parseExpectedName, *this, "Expected a name here.");
		return false;
	}

	if (this->string[0] == '"' || this->string[0] == '\'' || this->string[0] == '$')
	{
		errors->error(ErrorCode::parseExpectedName, *this, "Expected a name, got a string");
		return false;
	}

	bool isValid = true;

	for (auto c : this->string)
	{
		if (c != ':' && specialChars.find(c) != specialChars.end())
		{
			isValid = false;
		}
	}

	if (!isValid)
	{
		errors->error(ErrorCode::parseExpectedName, *this, "The name '" + this->string + "' contains invalid characters");
		return false;
	}
	return true;
}

static bool replace(std::string& str, const std::string& from, const std::string& to)
{
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}
bool ds::Token::unquoteString()
{
	if (string.size() < 2 || string[0] != '"' || string[string.size() - 1] != '"')
	{
		return false;
	}
	string = string.substr(1, string.size() - 2);
	replace(string, "\\n", "\n");
	return true;
}

void ds::TokenStream::fromFile(std::string path, ErrorContext* errors)
{
	std::ifstream file = std::ifstream(path);
	fromStream(file, path, errors);
	file.close();
}

void ds::TokenStream::fromString(const std::string& stringData, std::string path, ErrorContext* errors)
{
	std::stringstream stream;
	stream << stringData;
	fromStream(stream, path, errors);
}

void ds::TokenStream::fromTokens(const std::vector<Token> from)
{
	currentLine = &this->lineTokens.emplace_back();
	size_t lastLine = SIZE_MAX;
	for (auto& i : from)
	{
		if (lastLine != i.position.line && !currentLine->empty())
		{
			currentLine = &this->lineTokens.emplace_back();
		}
		lastLine = i.position.line;
		addToken(i);
	}
}

void ds::TokenStream::addLine()
{
	currentLine = &this->lineTokens.emplace_back();
}

void ds::TokenStream::fromStream(std::istream& stream, std::string name, ErrorContext* errors)
{
	Token currentWord = newToken();

	addLine();

	size_t bracketDepth = 0;

	bool inComment = false;

	while (true)
	{
		char newChar = getNextChar(stream);

		if (stream.eof() || newChar == EOF)
			break;

		if (newChar == '\n')
		{
			this->character = 0;
			this->line++;
			inComment = false;
			if (bracketDepth == 0)
			{
				addToken(currentWord);

				if (tryReadChar(stream, '{'))
				{
					auto charToken = newToken();
					charToken.addChar('{');
					addToken(charToken);
				}

				if (!currentLine->empty())
				{
					addLine();
				}
				currentWord = newToken();
				continue;
			}
		}

		if (inComment)
		{
			continue;
		}

		if (newChar == '"')
		{
			currentWord.addChar(newChar);
			do
			{
				newChar = getNextChar(stream);

				if (newChar == EOF)
				{
					errors->error(ErrorCode::tokenUnexpectedEof, currentWord, "Unexpected EOF");
					break;
				}

				if (newChar == '\\')
				{
					newChar = getNextChar(stream);

					if (newChar == '"')
					{
						currentWord.addChar('"');
						newChar = 0;
					}
					else
					{
						currentWord.addChar('\\');
					}
				}
				else
				{
					currentWord.addChar(newChar);
				}
			} while (newChar != '"');
			addToken(currentWord);
			currentWord = newToken();
			continue;
		}

		if (newChar == '/' && tryReadWord(stream, "//", 1))
		{
			inComment = true;
			continue;
		}

		if (specialChars.find(newChar) != specialChars.end() ||
			(newChar == '.' && !isNumber(currentWord.string, false)))
		{
			if (newChar == '}' && !currentLine->empty() && bracketDepth == 0)
			{
				addLine();
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

			if (newChar == '[' || newChar == '(')
			{
				bracketDepth++;
			}
			else if (bracketDepth > 0 && (newChar == ']' || newChar == ')'))
			{
				bracketDepth--;
			}
			addToken(currentWord);
			auto charToken = newToken();
			charToken.addChar(newChar);
			charToken.position.startPos--;
			charToken.position.endPos--;
			currentLine->push_back(charToken);
			if (newChar == '{' && bracketDepth == 0)
			{
				addLine();
			}
			currentWord = newToken();
			continue;
		}

		if (whitespace.find(newChar) != whitespace.end())
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

bool ds::TokenStream::tryReadWord(std::istream& stream, std::string word, size_t beginAt)
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

bool ds::TokenStream::tryReadChar(std::istream& stream, char c)
{
	StreamPosition startPos = StreamPosition(this, stream);
	while (true)
	{
		int newChar = stream.get();

		if (newChar == EOF)
			break;

		if (whitespace.find(newChar) != whitespace.end() || newChar == '\n')
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

void ds::TokenStream::readWhitespace(std::istream& stream)
{
	while (true)
	{
		int newChar = stream.get();
		if (newChar == EOF)
			break;

		if (whitespace.find(newChar) != whitespace.end())
		{
			character++;
			continue;
		}
		break;
	}
	stream.seekg(-1, stream.cur);
}

char ds::TokenStream::getNextChar(std::istream& stream)
{
	char newChar = stream.get();
	character++;
	return char(newChar);
}

TokenLine& ds::TokenLine::postProcessTokens(ErrorContext* errors)
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

void ds::TokenStream::addToken(const Token& target)
{
	if (!target.string.empty())
	{
		currentLine->push_back(target);
	}
}

const Token& ds::TokenLine::peek()
{
	if (this->empty())
		return EMPTY_TOKEN;
	return this->lineTokens->at(this->position);
}

const Token& ds::TokenLine::get()
{
	if (this->empty())
		return EMPTY_TOKEN;

	return this->lineTokens->at(this->position++);
}

bool ds::TokenLine::expect(std::string token, ErrorContext* errors)
{
	const Token& next = get();

	if (next.string.empty())
	{
		errors->error(ErrorCode::parseUnexpectedToken, next,
			"Expected '" + token + "'");
		return true;
	}


	if (next.string != token)
	{
		errors->error(ErrorCode::parseUnexpectedToken, next,
			"Expected '" + token + "', got '" + next.string + "'");
		return true;
	}
	return false;
}

bool ds::TokenLine::empty() const
{
	return !this->lineTokens || this->lineTokens->empty() || this->position >= this->lineTokens->size();
}

std::vector<Token> ds::TokenLine::getInBraces(ErrorContext* errors)
{
	std::vector<Token> result;

	size_t depth = 0;

	const Token& first = peek();

	do
	{
		const Token& next = get();

		if (next.empty())
		{
			errors->error(ErrorCode::parseUnexpectedToken, first, "No matching ')' found");
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

std::vector<Token> ds::TokenLine::getUntil(std::string token, ErrorContext* errors)
{
	std::vector<Token> result;

	size_t depth = 0;
	size_t scopeDepth = 0;
	while (true)
	{
		const Token& next = peek();

		if (next == "(")
		{
			depth++;
		}
		else if (next == ")")
		{
			if (depth == 0)
			{
				errors->error(ErrorCode::parseUnexpectedToken, next, "Unexpected ')'");
				break;
			}
			depth--;
		}

		if (next == "{" && token != "{")
		{
			scopeDepth++;
		}
		else if (next == "}")
		{
			if (scopeDepth == 0 && token != "}")
			{
				errors->error(ErrorCode::parseUnexpectedToken, next, "Unexpected '}'");
				break;
			}
			else if (scopeDepth > 0)
			{
				scopeDepth--;
			}
			// Skip last )
			if (scopeDepth == 0 && token != "}")
			{
				continue;
			}
		}

		if (next.string.size() && token.find(next.string[0]) == std::string::npos)
		{
			get();
		}
		else if ((depth == 0 && scopeDepth == 0) || next.empty())
		{
			break;
		}
		else
		{
			get();
		}

		result.push_back(next);
	}

	get();
	return result;
}

bool ds::TokenLine::contains(std::string token) const
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

const Token& ds::TokenLine::previous()
{
	return this->lineTokens && this->lineTokens->size()
	           ? this->lineTokens->at(this->position - 1)
	           : EMPTY_TOKEN;
}

size_t ds::TokenLine::savePosition() const
{
	return this->position;
}

void ds::TokenLine::loadPosition(size_t oldPos)
{
	this->position = oldPos;
}

void ds::TokenLine::expectEndOfLine(ErrorContext* errors)
{
	if (!empty())
	{
		errors->error(ErrorCode::parseUnexpectedToken, peek(), "Unexpected '" + peek().string + "'");
	}
}

TokenLine ds::TokenStream::next(ErrorContext* errors)
{
	if (currentStreamLine == this->lineTokens.size())
	{
		return TokenLine();
	}
	return TokenLine{
		.lineTokens = &this->lineTokens.at(currentStreamLine++)
	}
	    .postProcessTokens(errors);
}

TokenLine ds::TokenStream::peek(ErrorContext* errors)
{
	if (currentStreamLine == this->lineTokens.size())
	{
		return TokenLine();
	}
	return TokenLine{
		.lineTokens = &this->lineTokens[currentStreamLine]
	}
	    .postProcessTokens(errors);
}

void ds::TokenStream::addLine(TokenLine ln)
{
	this->lineTokens.push_back(*ln.lineTokens);
}

void ds::TokenStream::getScope(TokenStream& addTo, ErrorContext* errors, size_t beginDepth)
{
	bool isFirst = addTo.lineTokens.empty();
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

		if (isFirst)
		{
			addTo.first = functionLine.peek().position;
			isFirst = false;
		}
		addTo.last = functionLine.lineTokens->at(functionLine.lineTokens->size() - 1).position;

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

Token ds::TokenStream::newToken() const
{
	TokenPos p;

	p.startPos = this->character;
	p.endPos = this->character;
	p.line = this->line;

	return Token("", p);
}

TokenStream::StreamPosition::StreamPosition(TokenStream* from, std::istream& stream)
{
	this->position = stream.tellg();
	this->character = from->character;
	this->line = from->line;
}

void ds::TokenStream::StreamPosition::apply(TokenStream* to, std::istream& stream) const
{
	stream.seekg(this->position);
	to->character = this->character;
	to->line = this->line;
}
