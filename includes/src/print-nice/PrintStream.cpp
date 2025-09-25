#include <iostream>
#include "PrintStream.h"
#include "PrintNice.h"

PrintStream &operator<<(PrintStream &stream, TextStyledToken token)
{
	stream.add(token);
	return stream;
}

PrintStream &operator<<(PrintStream &stream, std::string& token)
{
	stream.add(TextStyledToken{token.c_str(), OutputType::Normal, 0});
	return stream;
}

PrintStream &operator<<(PrintStream &stream, const char *const token)
{
	stream.add(TextStyledToken{token, OutputType::Normal, 0});
	return stream;
}

PrintStream &operator<<(PrintStream &stream, StreamOut)
{
	stream.print();
	return stream;
}

void PrintStream::add(std::string token)
{
	this->add(token.c_str());
}

void PrintStream::add(const char *token)
{
	this->tokens.push_back({token, OutputType::Normal, 0});
}

void PrintStream::add(TextStyledToken token)
{
	this->tokens.push_back(token);
}

void PrintStream::print()
{
	for (TextStyledToken& token: this->tokens) {
		auto [text, type, style] = token;
		TextStyled s = PrintNice::styled(text, type, style);
		std::cout << s << this->separator;
	}

	std::cout << std::endl;

	this->tokens.clear();
}

StreamOut::StreamOut()
{
}
