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
	SharedLib = 4,
	Composed = 5
};

struct PackageData {
	std::string name;
	std::string version;
	std::string path;
	PackageType type;
	std::vector<std::string> tests;
	std::vector<std::string> includeDirs; // only used for composed packages
	std::vector<std::string> libDirs; // only used for composed packages
	std::vector<std::string> linkableObjects;
	std::vector<std::string> dependencies;
	bool managed;
	int registeredAt;
};

struct DependencyTarget {
	std::filesystem::path from;
	std::filesystem::path to;
};

template <typename T, unsigned int S>
using SmartArray = std::array<T, S>;

using Empty = const char*;

template <typename T>
using Maybe = std::variant<T, Empty>;

enum BuildConfig {
	Debug,
	Release
};