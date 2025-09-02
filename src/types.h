#pragma once

#include <string>
#include <vector>

enum SystemType {
	Unix,
	Windows
};

enum PackageType {
	ConsoleApp = 1,
	WindowedApp = 2,
	StaticLib = 3,
	SharedLib = 4
};

struct PackageData {
	std::string name;
	std::string version;
	std::string path;
	PackageType type;
	std::vector<std::string> linkableObjects;
	std::vector<std::string> dependencies;
	bool managed;
	int registeredAt;
};