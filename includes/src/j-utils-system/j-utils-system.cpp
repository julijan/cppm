#include <filesystem>
#include "j-utils-system.h"

namespace utils {

	namespace system {

		SystemType type() {
			if (std::filesystem::exists("/etc")) {
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

		int runCommand(const std::string& command) {
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
}