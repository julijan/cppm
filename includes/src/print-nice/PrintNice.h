#pragma once

#include "Types.h"
#include "TextStyled.h"
#include "PrintStream.h"

class PrintNice {

public:
	// print given TextStyled
	static void print(const TextStyled& t);

	// print given text with a normal style using color based on type
	static void print(const char* text, OutputType type);

	// print given text with a normal style using color based on type
	static void print(const std::string& text, OutputType type);

	// print given text with given type and style
	// style can be constructed using bitwise or of TextStyle
	static void print(const char* text, OutputType type, int style);

	// prints a newline character
	static void print();

	// print text using OutputType::Warning
	static void warning(const char* text);
	// print text using OutputType::Warning
	static void warning(const std::string& text);

	// print text using OutputType::Info
	static void info(const char* text);
	// print text using OutputType::Info
	static void info(const std::string& text);

	// print text using OutputType::Success
	static void success(const char* text);
	// print text using OutputType::Success
	static void success(const std::string& text);

	// print text using OutputType::Error
	static void error(const char* text);
	// print text using OutputType::Error
	static void error(const std::string& text);

	// print given text using OutputType::Error and additional style based on severity
	static void error(const char* text, ErrorSeverity severity);
	// print given text using OutputType::Error and additional style based on severity
	static void error(const std::string& text, ErrorSeverity severity);

	// construct TextStyled setting the color and style based on provided arguments
	static TextStyled styled(OutputType type, int style);

	// construct TextStyled setting the color and style based on provided arguments setting it's contents
	static TextStyled styled(const char* text, OutputType type, int style);

	// returns instance of PrintStream
	static PrintStream stream();

private:
	// returns a color as a hex string provided OutputType
	static const char* typeColor(OutputType t);
};