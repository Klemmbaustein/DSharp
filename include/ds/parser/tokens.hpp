#pragma once
#include <string>
#include <cstdint>
#include <fstream>
#include <vector>

namespace ds
{
	struct ErrorContext;
	struct TokenPos
	{
		uint32_t startPos = 0;
		uint32_t endPos = 0;
		uint32_t line = 0;
	};

	struct Token
	{
		Token(std::string str)
		{
			this->string = str;
		}

		Token(const char* str)
		{
			this->string = str;
		}

		Token()
		{
		}

		Token(std::string str, TokenPos pos)
		{
			this->string = str;
			this->position = pos;
		}

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

		bool operator!=(const char* str) const
		{
			return string != str;
		}

		void addChar(char c);
		void addStr(std::string str);

		void merge(const Token& other);

		bool unquoteString();

		bool operator<(const Token& other) const
		{
			return this->string < other.string;
		}

		bool checkIsName(ErrorContext* errors) const;
	};

	struct TokenLine
	{
		const Token& peek();
		const Token& get();
		bool expect(std::string token, ErrorContext* errors);
		bool empty() const;

		std::vector<Token> getInBraces(ErrorContext* errors);
		std::vector<Token> getUntil(std::string token, ErrorContext* errors, bool allowNoFind = false);

		bool contains(std::string token) const;

		const Token& previous();

		size_t savePosition() const;

		void loadPosition(size_t oldPos);

		void expectEndOfLine(ErrorContext* errors);

		TokenLine& postProcessTokens(ErrorContext* errors);

		std::vector<Token>* lineTokens = nullptr;
		size_t position = 0;
	};

	struct TokenStream
	{
		void reset()
		{
			currentStreamLine = 0;
		}
		void fromFile(std::string path, ErrorContext* errors);
		void fromStream(std::istream& stream, std::string name, ErrorContext* errors);
		void fromString(const std::string& stringData, std::string path, ErrorContext* errors);

		void fromTokens(const std::vector<Token> from);

		TokenLine next(ErrorContext* errors);
		TokenLine peek(ErrorContext* errors);
		void addLine(TokenLine ln);
		void getScope(TokenStream& addTo, ErrorContext* errors, size_t beginDepth = 1);
		uint32_t character = 0, line = 0;

		TokenPos first;
		TokenPos last;

		void addLine();
		void addToken(const Token& target);

	private:
		std::vector<std::vector<Token>> lineTokens;
		std::vector<Token>* currentLine = nullptr;
		size_t currentStreamLine = 0;

		bool tryReadWord(std::istream& stream, std::string word, size_t beginAt);

		bool tryReadChar(std::istream& stream, char c);
		void readWhitespace(std::istream& stream);
		char getNextChar(std::istream& stream);


		Token newToken() const;

		struct StreamPosition
		{
			StreamPosition(TokenStream* from, std::istream& stream);

			uint32_t character = 0, line = 0;
			std::streampos position = 0;

			void apply(TokenStream* to, std::istream& stream) const;
		};
	};
} // namespace ds