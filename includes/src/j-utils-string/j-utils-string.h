#pragma once

#include <string>
#include <vector>
#include <cstring>

namespace utils {

	namespace string {

		// replace all occurrences of needle with replaceWith
		std::string replaceAll(
			std::string str,
			const char* needle,
			const char* replaceWith
		);

		std::string replaceAll(
			const char* str,
			const char* needle,
			const char* replaceWith
		);

		// remove space, newline and tab chars from beginning and end of given string
		// returns a new string
		std::string trim(const std::string& str);

		// used to display large blocks of text in a nicer format
		// toWidth will insert a line break every charWidth chars, unless strict = false
		// in which case it will avoid breaking words while still trying to get close to desired width
		std::string toWidth(const std::string str, unsigned int charWidth, bool strict);

		// split given string at given separator, separator can be of any length
		std::vector<std::string> split(const std::string& str, const std::string& separator);

		// repeat given string repeatCount times
		std::string repeat(const char* str, unsigned int repeatCount);

		// if you have C++20 available, use ends_with shipped with std::string
		bool endsWith(const std::string& string, const std::string& endChars);
	}

}