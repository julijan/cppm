#pragma once

#include <string>
#include <filesystem>
#include "boost/json.hpp"

#include "types.h"

namespace utils {
	namespace string {

		// replace all occurences of needle with replaceWith
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

		// used to disaply large blocks of text in a nicer format
		// toWidth will insert a line break every charWidth chars, unless strict = false
		// in which case it will avoid breaking words while still trying to get close to desired width
		std::string toWidth(const std::string str, unsigned int charWidth, bool strict);

		// split given string at given separator, separator can be of any length
		std::vector<std::string> split(const std::string& str, const std::string& separator);
	}

	namespace fs {
		using namespace std::filesystem;

		bool pathExists(path p);

		void mkdir(path p);

		path currentPath();

		// append all given sub-directories to given path
		// returns a new path
		template <int Depth>
		inline std::filesystem::path extendPath(const std::filesystem::path& p, SmartArray<const char*, Depth> subdirs) {
			std::filesystem::path path(p);

			for (const char* subdir: subdirs) {
				path.append(subdir);
			}

			return path;
		};
	}

	namespace system {
		SystemType type();

		int runCommand(const char* const command);
		int runCommand(std::string command);

		// return system user, used for accessing the home directory
		std::string user();

		std::string appDataDir();
	}

	namespace json {
		void write(const std::filesystem::path& file, const boost::json::value& json);
		boost::json::value read(const std::filesystem::path& file);
	}

	namespace time {
		int unixTimestamp();
	}
}