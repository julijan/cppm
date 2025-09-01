#include <fstream>
#include <iostream>
#include <istream>
#include <vector>
#include "core.h"
#include "package.h"
#include "boost/json.hpp"

std::filesystem::path Core::path() {
	return utils::system::appDataDir() + std::filesystem::path::preferred_separator + ".cppm";
}

void Core::initDirectory()
{
	if (!Core::directoryExists()) {
		utils::fs::mkdir(Core::path());
		Core::initRegistry();
	}
}

bool Core::directoryExists()
{
	return utils::fs::pathExists(Core::path());
}

std::filesystem::path Core::filePath(const char* const fileName) {
	return std::filesystem::path(Core::path()).append(fileName);
}


void Core::initRegistry() {
	std::ofstream fh(Core::filePath("registry.json"));
	boost::json::array registry = {};

	if (!fh.is_open()) {
		std::cerr << "Error initializing package registry" << std::endl;
		return;
	}
	
	fh << registry;
}

void Core::printHelp(const char* command)
{
	if (command == nullptr) {
		// general help, list commands
		std::cout << "Usage cppm [command] [...options]" << std::endl << std::endl;
		std::cout << "Commands:" << std::endl;
		std::cout << "init" << std::endl;
	}
}