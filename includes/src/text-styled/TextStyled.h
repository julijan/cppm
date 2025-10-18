#pragma once

#include <string>
#include <ostream>

class TextStyled {
public:
	TextStyled();
	TextStyled(std::string& str);
	TextStyled(const char * str);

	TextStyled& text(const std::string& text);
	TextStyled& text(const char* text);

	TextStyled& bold(bool val);
	TextStyled& bold();

	TextStyled& italic(bool val);
	TextStyled& italic();

	TextStyled& underline(bool val);
	TextStyled& underline();

	TextStyled& color(int r, int g, int b);

	// eg. #FF0000 or FF0000
	TextStyled& color(const char* hex);

	std::string output() const;

private:
	std::string str;

	// text properties
	bool _bold;
	bool _italic;
	bool _underline;

	int _color[3];

	void setDefaults();
};

std::ostream& operator<<(std::ostream& stream, const TextStyled& text);