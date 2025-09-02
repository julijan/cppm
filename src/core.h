#pragma once

#include <filesystem>
#include <string>

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
	
	// returns file path within ~/.cppm directory
	static std::filesystem::path filePath(const char* const fileName);
private:
	// check if ~/.cppm directory exists
	static bool directoryExists();
};