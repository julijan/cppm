#pragma once

#include <string>
#include <filesystem>
#include <functional>
#include <algorithm>

#include "types.h"

namespace utils {

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

		// return a vector of all paths for which the predicate returns true
		// predicate will receive all paths recursively, starting from the entry
		std::vector<path> filterRecursive(path entry, std::function<bool(const path& path)> predicate);
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