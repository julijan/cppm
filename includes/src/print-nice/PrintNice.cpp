#include <iostream>

#include "PrintNice.h"

void PrintNice::print(const TextStyled &t)
{
	std::cout << t << std::endl;
}

void PrintNice::print(const char *text, OutputType type)
{
	PrintNice::print(text, type, 0);
}

void PrintNice::print(const char *text, OutputType type, int style)
{
	// set color based on type
	TextStyled t = PrintNice::styled(text, type, style);

	// print
	PrintNice::print(t);
}

void PrintNice::print()
{
	PrintNice::print("");
}

void PrintNice::warning(const char *text)
{
	PrintNice::print(text, OutputType::Warning);
}

void PrintNice::warning(const std::string &text)
{
	PrintNice::print(text.c_str(), OutputType::Warning);
}

void PrintNice::info(const char *text)
{
	PrintNice::print(text, OutputType::Info);
}

void PrintNice::info(const std::string &text)
{
	PrintNice::print(text.c_str(), OutputType::Info);
}

void PrintNice::success(const char *text)
{
	PrintNice::print(text, OutputType::Success);
}

void PrintNice::success(const std::string &text)
{
	PrintNice::print(text.c_str(), OutputType::Success);
}

void PrintNice::print(const std::string &text, OutputType type)
{
	PrintNice::print(text.c_str(), type);
}

void PrintNice::error(const char *text)
{
	PrintNice::print(text, OutputType::Error);
}

void PrintNice::error(const std::string &text)
{
	PrintNice::error(text.c_str());
}

void PrintNice::error(const char *text, ErrorSeverity severity)
{
	PrintNice::print(text, OutputType::Error, severity & 7);
}

void PrintNice::error(const std::string &text, ErrorSeverity severity)
{
	PrintNice::error(text.c_str(), severity);
}

const char *PrintNice::typeColor(OutputType t)
{

	if (t == OutputType::Success) {
		return "#00E86B";
	}

	if (t == OutputType::Info) {
		return "#00C4E8";
	}

	if (t == OutputType::Warning) {
		return "#E8CD00";
	}

	if (t == OutputType::Error) {
		return "#E80400";
	}

	return "#F6F6F6";
}

TextStyled PrintNice::styled(OutputType type, int style)
{
	TextStyled t;

	t.color(PrintNice::typeColor(type));
	
	t.bold(style & TextStyle::Bold);
	t.italic(style & TextStyle::Italic);
	t.underline(style & TextStyle::Underline);

	return t;
}

TextStyled PrintNice::styled(const char *text, OutputType type, int style)
{
	TextStyled t = PrintNice::styled(type, style);
	t.text(text);
	return t;
}

PrintStream PrintNice::stream()
{
	return PrintStream();
}
