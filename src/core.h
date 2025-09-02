#pragma once

#include <filesystem>
#include <string>
#include "utils.h"

struct Command {
	std::string name;
	std::string description;
	const char* options[];
};

class Core {
public:
	// ~/.cppm
	static std::filesystem::path path();

	// create ~/.cppm directory if it does not exist
	static void initDirectory();
	
	static std::filesystem::path filePath(const char* const fileName);
	
	static void printHelp(const char* command = nullptr);

	private:

	// check if ~/.cppm directory exists
	static bool directoryExists();
};