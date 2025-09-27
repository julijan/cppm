#pragma once

#include <string>
#include <filesystem>
#include <functional>
#include <algorithm>
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

		// run command and return exit code
		int runCommand(const char* const command);
		int runCommand(std::string command);

		// execute command and return stdout output
		std::string runCommandOutput(std::string command);

		// return system user, used for accessing the home directory
		std::string user();

		std::string appDataDir();
	}

	namespace json {
		void write(const std::filesystem::path& file, const boost::json::value& json);
		boost::json::value read(const std::filesystem::path& file);

		// produce a boost::json::array from given vector
		boost::json::array toJSONArray(const std::vector<std::string>& arr);

		// produce a vector from given boost::json::array
		std::vector<std::string> fromJSONArray(const boost::json::array& arr);

		// safely extract array from given JSON object
		// if key exists in given boost::json::object, returns vector from it
		// if key does not exist, returns an empty vector
		std::vector<std::string> extractArrayFromJSONObject(const boost::json::object& obj, const char* key);
	}

	namespace vector {
		// join vector elements using provided glue
		std::string join(const std::vector<std::string>& vec, const std::string& glue);

		// transform each element running it through a predicate
		// retuning a new vector of transformed elements
		template <typename R, typename T>
		std::vector<R> map(const std::vector<T>& vec, std::function<R(const T&)> pred)
		{
			std::vector<std::string> res;
			std::transform(vec.begin(), vec.end(), std::back_inserter(res), pred);
			return res;
		}

		// returns a string: "el0", "el1", "el2", ...
		std::string toQuotedList(const std::vector<std::string>& vec);
	}

	namespace time {
		int unixTimestamp();
	}
}