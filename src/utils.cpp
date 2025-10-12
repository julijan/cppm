#include <functional>
#include <string>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <vector>

#include "utils.h"

namespace utils {

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

		std::vector<path> filterRecursive(path entry, std::function<bool(const path& path)> predicate) {
			std::vector<path> results;

			if (predicate(entry)) {
				// entry itself matches predicate
				results.push_back(entry);
			}

			// scan entry
			directory_iterator iter(entry);
			for (auto entry: iter) {
				path entryPath = entry.path();

				// path in entry matches predicate
				if (predicate(entryPath)) {
					results.push_back(entryPath);
				}

				if (is_directory(entryPath)) {
					// descend recursively
					std::vector<path> resultsNext = filterRecursive(entryPath, predicate);
					results.insert(results.begin(), resultsNext.begin(), resultsNext.end());
				}
			}

			return results;
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
			int exitCode = std::system(command);
			if (type() == SystemType::Unix) {
				// on Unix-like systems, exit code is shifted 8 bits to the left
				// shift right to return the raw exit code
				return exitCode >> 8;
			}
			return exitCode;
		}

		int runCommand(std::string command) {
			return runCommand(command.c_str());
		}

		std::string runCommandOutput(std::string command)
		{
			FILE* pipe = popen(command.c_str(), "r");

			if (!pipe) {
				throw std::runtime_error("Error running command: " + command);
			}

			char buffer[128];
			std::string result = "";
			while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
				result += buffer;
			}
			pclose(pipe);

			return result;
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

std::string utils::vector::join(const std::vector<std::string>& vec, const std::string& glue)
{
	std::string joined = "";
	for (auto i = vec.cbegin(); i != vec.cend(); ++i) {
		joined += *i;
		if (i != vec.cend() - 1) {
			joined += glue;
		}
	}
	return joined;
}

std::string utils::vector::toQuotedList(const std::vector<std::string>& vec)
{
	return utils::vector::join(
		utils::vector::map<std::string, std::string>(vec, [](const std::string& item) {
			std::string out = "";
			out += '"' + item + '"';
			return out;
		}),
		", "
	);
}
