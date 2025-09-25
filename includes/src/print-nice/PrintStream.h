#pragma once

#include <tuple>
#include <vector>
#include <string>
#include <ostream>

#include "Types.h"

// if streamed into PrintStream instance, PrintStream.print will get executed
class StreamOut {
public:
	StreamOut();
};

class PrintStream {
public:
	// tokens will be separated by separator
	std::string separator = " ";

	// add token
	void add(std::string token);
	void add(const char* token);
	void add(TextStyledToken token);

	// write tokens to stdout and clear tokens
	void print();

private:
	// stored tokens
	std::vector<TextStyledToken> tokens;
};

// stream TextStyledToken into PrintStream
PrintStream& operator<<(PrintStream& stream, TextStyledToken token);

// stream text into PrintStream, assumes OutputType::Normal and no styling
PrintStream& operator<<(PrintStream& stream, std::string& token);
PrintStream& operator<<(PrintStream& stream, const char* const token);

// write tokens to stdout, same as PrintStream.print()
PrintStream& operator<<(PrintStream& stream, StreamOut);