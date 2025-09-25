#pragma once

class PrintNice;
class PrintStream;

enum OutputType {
	Normal,
	Success,
	Info,
	Warning,
	Error
};

enum TextStyle {
	Bold = 1,
	Italic = 2,
	Underline = 4
};

enum ErrorSeverity {
	Low = 0, // normal red text
	Medium = 1, // bold red text
	High = 5 // underlined bold red text
};

// text, type and style/style combination
struct TextStyledToken {
	const char* text;
	OutputType type;
	int style;
};