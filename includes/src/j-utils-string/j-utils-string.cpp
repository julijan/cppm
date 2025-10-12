#include "j-utils-string.h"

namespace utils {
	namespace string {
		std::string replaceAll(
			std::string str,
			const char* needle,
			const char* replaceWith
		) {
			while (true)
			{
				size_t index = str.find(needle);
				if (index == str.npos) {break;}

				str.replace(index, strlen(needle), replaceWith);
			}
			return str;
		}

		std::string replaceAll(
			const char* str,
			const char* needle,
			const char* replaceWith
		) {
			std::string s(str);
			return replaceAll(s, needle, replaceWith);
		}

		std::string trim(const std::string& str) {
			std::string out = "";

			int start = 0;
			int end = 0;
			for (int i = 0; i < str.length(); i++) {
				if (str[i] == ' ' || str[i] == '\n' || str[i] == '\t') {continue;}
				start = i;
				break;
			}
			
			for (int i = str.length() - 1; i > -1; i--) {
				if (str[i] == ' ' || str[i] == '\n' || str[i] == '\t') {continue;}
				end = i;
				break;
			}

			if (start == end) {return out;}

			return str.substr(start, end + 1);
		}

		std::string toWidth(const std::string str, unsigned int charWidth, bool strict)
		{
			std::string output = "";
			int currentChar = 0;
			int lineWidth = 0;
			while (currentChar < str.length()) {
				char c = str[currentChar];

				if (c == '\n') {
					// input string contains a line break
					// reset lineWidth
					lineWidth = 0;
				} else if (lineWidth > charWidth) {
					// time to insert a line break
					// it will be done in all cases if strict = true
					// otherwise only if a bounds char is found
					bool boundsChar = c == ' ' || c == '\t';
					bool breakLine = boundsChar || strict;
					if (breakLine) {
						output += '\n';
						lineWidth = 0;
						if (!strict) {
							// skip the current char, it is a bounds char in this case
							currentChar++;
							continue;
						}
					}
				}

				output += c;

				lineWidth++;
				currentChar++;
			}

			return output;
		}

		std::vector<std::string> split(const std::string &str, const std::string &separator)
		{
			std::vector<std::string> tokens;
			std::string token;
			std::string possiblySeparator;

			int separatorCharsMatched = 0;

			for (int i = 0; i < str.length(); i++) {
				char c = str[i];

				if (separator.length() == 0) {
					// empty separator, always splits
					token = c;
					tokens.push_back(token);
					token.clear();
					continue;
				}

				char expectedSeparatorChar = separator[separatorCharsMatched];

				if (c == expectedSeparatorChar) {
					// resuming potential separator
					separatorCharsMatched++;
					possiblySeparator += c;

					if (separatorCharsMatched == separator.length()) {
						// entire separator matched
						separatorCharsMatched = 0;
						tokens.push_back(token);
						token.clear();
						possiblySeparator.clear();
						continue;
					}

					continue;
				} else {
					// separator sequence broken
					token += possiblySeparator;
					separatorCharsMatched = 0;
					possiblySeparator.clear();
				}

				token += c;
			}

			tokens.push_back(token);

			return tokens;
		}

		std::string repeat(const char* str, unsigned int repeatCount)
		{
			std::string out = "";
			for (int i = 0; i < repeatCount; ++i) {
				out += str;
			}
			return out;
		}
	}
}