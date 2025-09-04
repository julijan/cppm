#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <chrono>

#include "utils.h"

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

		std::string trim(std::string& str) {
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
	}

	namespace fs {
		using namespace std::filesystem;

		bool pathExists(path p) {
			return exists(p);
		}

		void mkdir(path p)
		{
			create_directory(p);
		}

		path currentPath()
		{
			return current_path();
		}
	}

	namespace system {

		SystemType type() {
			if (utils::fs::pathExists("/etc")) {
				return SystemType::Unix;
			}
			return SystemType::Windows;
		}

		int runCommand(const char* const command) {
			return std::system(command);
		}

		int runCommand(std::string command) {
			return runCommand(command.c_str());
		}

		std::string user() {
			if (type() == SystemType::Unix) {
				return std::getenv("USER");
			}
			return std::getenv("USERPROFILE");
		}

		std::string appDataDir() {
			if (type() == SystemType::Unix) {
				return "/home/" + user();
			}
			return "C:\\Users\\" + user() + "\\Local";
		}

	}

	namespace json {
		void write(const std::filesystem::path& file, const boost::json::value& json)
		{
			std::ofstream fh(file);
			if (!fh.is_open()) {
				std::cerr << "Error initializing package registry" << std::endl;
				return;
			}
			fh << json;

			fh.flush();
			fh.close();
		}

		boost::json::value read(const std::filesystem::path &file)
		{
			if (!std::filesystem::exists(file)) {
				return boost::json::value();
			}

			boost::json::stream_parser parser;
			std::fstream stream(file);

			if (!stream.is_open()) {
				std::cerr << "Error opening JSON file for reading" << std::endl;
				return boost::json::array();
			}

			std::string line;
			while (std::getline(stream, line)) {
				parser.write(line);
			}
			parser.finish();

			return parser.release();
		}
	}

	namespace time {
		int unixTimestamp() {
			auto now = std::chrono::system_clock::now();
			auto sinceEpoch = now.time_since_epoch();
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch);
			return seconds.count();
		}
	}

}