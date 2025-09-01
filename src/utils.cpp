#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <filesystem>
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

	namespace time {
		int unixTimestamp() {
			auto now = std::chrono::system_clock::now();
			auto sinceEpoch = now.time_since_epoch();
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch);
			return seconds.count();
		}
	}

}