#pragma once

#include <string>

enum class SystemType {
	Unix,
	Windows
};

namespace utils {
	namespace system {
		SystemType type();

		// run command and return exit code
		int runCommand(const char* const command);
		int runCommand(const std::string& command);

		// execute command and return stdout output
		std::string runCommandOutput(std::string command);

		// return system user, used for accessing the home directory
		std::string user();

		std::string appDataDir();
	}
}