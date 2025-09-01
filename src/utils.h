#pragma once

#include <string>
#include "types.h"

namespace utils {
	namespace string {

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

	}

	namespace fs {
		using namespace std::filesystem;

		bool pathExists(path p);

		void mkdir(path p);

		path currentPath();
	}

	namespace system {
		SystemType type();

		int runCommand(const char* const command);
		int runCommand(std::string command);

		// return system user, used for accessing the home directory
		std::string user();

		std::string appDataDir();
	}

	namespace time {
		int unixTimestamp();
	}
}