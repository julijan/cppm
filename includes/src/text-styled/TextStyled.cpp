#include <iostream>
#include <cstring>
#include <cmath>

#include "TextStyled.h"

TextStyled::TextStyled()
{
	this->str = "";
	this->setDefaults();
}

TextStyled::TextStyled(std::string &str)
{
	this->str = str;
	this->setDefaults();
}

TextStyled::TextStyled(const char *str)
{
	this->str = str;
	this->setDefaults();
}

TextStyled &TextStyled::text(const std::string &text)
{
	this->str = text;
	return *this;
}

TextStyled &TextStyled::text(const char *text)
{
	this->str = text;
	return *this;
}

TextStyled& TextStyled::bold(bool val)
{
	this->_bold = val;
	return *this;
}

TextStyled& TextStyled::bold()
{
	return this->bold(true);
}

TextStyled& TextStyled::italic(bool val)
{
	this->_italic = val;
	return *this;
}

TextStyled &TextStyled::italic()
{
	return this->italic(true);
}

TextStyled& TextStyled::underline(bool val)
{
	this->_underline = val;
	return *this;
}

TextStyled &TextStyled::underline()
{
	return this->underline(true);
}

TextStyled &TextStyled::color(int r, int g, int b)
{
	this->_color[0] = r;
	this->_color[1] = g;
	this->_color[2] = b;
	return *this;
}

TextStyled &TextStyled::color(const char *hex)
{
	int len = strlen(hex);
	if (len < 6 || len > 7) {
		// expected 6 or 7 characters
		return *this;
	}

	int nibbleMS = 0;

	int r = 0;
	int g = 0;
	int b = 0;

	for (int i = (len == 7 ? 1 : 0); i < len; i++) {
		char c = hex[i];

		int nibbleAbs = i - (len == 7 ? 1 : 0);
		int component = nibbleAbs / 2;
		int nibbleIsMS = nibbleAbs % 2 == 0;

		int nibbleValue;

		if (c > 47 && c < 58) {
			// 0-9
			nibbleValue = c - '0';
		} else if (c > 64 && c < 71) {
			// A-F
			nibbleValue = 10 + c - 'A';
		} else if (c > 96 && c < 103) {
			// a-f
			nibbleValue = 10 + c - 'a';
		} else {
			// unexpected char
			nibbleValue = 0;
		}

		if (nibbleIsMS) {
			// processed nibble MS, store
			nibbleMS = nibbleValue;
		} else {
			int val = (nibbleMS << 4) | nibbleValue;

			// processed nibble LS, set component value
			if (component == 0) {
				// red
				r = val;
			} else if (component == 1) {
				g = val;
			} else {
				b = val;
			}
		}
	}

	// set the color
	this->color(r, g, b);

	return *this;
}

std::string TextStyled::output() const
{
	// start ASNI sequence
	std::string out = "\033[";

	// bold
	if (this->_bold) {
		out += "1;";
	}

	// italic
	if (this->_italic) {
		out += "3;";
	}

	// underline
	if (this->_underline) {
		out += "4;";
	}

	// set color
	out += "38;2;";
	out += std::to_string(this->_color[0]);
	out += ";";
	out += std::to_string(this->_color[1]);
	out += ";";
	out += std::to_string(this->_color[2]);

	// finish ANSI sequence
	out += 'm';
	out += this->str;
	out += "\033[0m";
	
	return out;
}

void TextStyled::setDefaults()
{
	this->_bold = false;
	this->_italic = false;
	this->_underline = false;
	this->_color[0] = 255;
	this->_color[1] = 255;
	this->_color[2] = 255;
}

std::ostream &operator<<(std::ostream &stream, const TextStyled &text)
{
	stream << text.output();
	return stream;
}
