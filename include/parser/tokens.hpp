#pragma once
#include <string>
#include <cstdint>
#include <fstream>
#include <vector>

namespace lang
{
	struct ErrorContext;
	struct TokenPos
	{
		uint32_t startPos = 0;
		uint32_t endPos = 0;
		uint64_t line = 0;
	};

	struct Token
	{
		std::string string;
		TokenPos position;

		bool empty() const
		{
			return string.empty();
		}

		bool operator==(const char* str) const
		{
			return string == str;
		}

		void addChar(char c);
		void addStr(std::string str);

		void merge(const Token& other);

		bool operator<(const Token& other) const
		{
			return this->string < other.string;
		}
	};

	struct TokenLine
	{
		Token peek();
		Token get();
		bool empty() const;

		std::vector<Token> getInBraces(ErrorContext* errors);
		std::vector<Token> getUntil(std::string token, ErrorContext* errors);

		bool contains(std::string token) const;

		Token previous();

		size_t savePosition();

		void loadPosition(size_t oldPos);

		void expectEndOfLine(ErrorContext* errors);

		TokenLine& postProcessTokens(ErrorContext* errors);

		std::vector<Token>* lineTokens = nullptr;
		size_t position = 0;
	};

	struct TokenStream
	{
		void fromFile(std::string path, ErrorContext* errors);
		void fromStream(std::istream& stream, std::string name, ErrorContext* errors);

		TokenLine next(ErrorContext* errors);
		TokenLine peek(ErrorContext* errors);
		void addLine(TokenLine ln);
		void getScope(TokenStream& addTo, ErrorContext* errors, size_t beginDepth = 1);

	private:
		std::vector<std::vector<Token>> lineTokens;
		std::vector<Token>* currentLine = nullptr;
		uint32_t character = 0, line = 0;
		size_t currentStreamLine = 0;

		bool tryReadWord(std::istream& stream, std::string word, size_t beginAt);

		bool tryReadChar(std::istream& stream, char c);
		void readWhitespace(std::istream& stream);
		char getNextChar(std::istream& stream);

		void addToken(Token& target);

		Token newToken();

		struct StreamPosition
		{
			StreamPosition(TokenStream* from, std::istream& stream);

			uint32_t character = 0, line = 0;
			std::streampos position = 0;

			void apply(TokenStream* to, std::istream& stream) const;
		};
	};
} // namespace lang