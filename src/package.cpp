#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <vector>
#include <string>

#include "package.h"
#include "utils.h"
#include "templates.h"
#include "boost/json.hpp"
#include "core.h"

Package::Package(std::string name, std::string path, PackageType type, bool managed)
{
	this->name = name;
	this->path = path;
	this->type = type;
	this->managed = managed;
	this->registeredAt = utils::time::unixTimestamp();
	this->linkableObjects = std::vector<std::string>();
	this->linkableObjects = std::vector<std::string>();
}

Package::Package(
	std::string name,
	std::string path,
	PackageType type,
	std::string version,
	std::vector<std::string> linkableObjects,
	std::vector<std::string> dependencies,
	bool managed,
	int registeredAt)
{
	this->name = name;
	this->path = path;
	this->type = type;
	this->version = version;
	this->linkableObjects = linkableObjects;
	this->dependencies = dependencies;
	this->managed = managed;
	this->registeredAt = registeredAt;
};

void Package::create(const char *const name)
{

	if (Package::packageExists(name)) {
		std::cerr << "Package with name " << name << " already exists" << std::endl;
		return;
	}

	// get current path
	const std::filesystem::path cwd = std::filesystem::current_path();

	// path to be created
	const std::filesystem::path projectDir = std::filesystem::path(cwd).append(name);

	// check if exists
	if (exists(projectDir)) {
		std::cerr << "Path " << projectDir.c_str() << " exists" << std::endl;
		return;
	}

	// project dir does not exist, create it
	try {
		create_directory(projectDir);
	} catch(std::filesystem::filesystem_error e) {
		std::cerr << "Error creating project directory: " << e.what() << std::endl;
	}

	// define created subdirectories
	const char* const paths[] = {
		"src",
		"header",
		"includes"
	};

	// create subdirectories
	for (auto p: paths) {
		const std::filesystem::path current = std::filesystem::path(projectDir).append(p);
		
		try {
			create_directory(current);
		} catch(std::filesystem::filesystem_error e) {
			std::cerr << "Error creating " << p << " directory: " << e.what() << std::endl;
		}
	}

	// select project type
	std::string pType = Package::promptType();

	// create premake5.lua
	std::string premakeFinal = utils::string::replaceAll(Templates::PREMAKE, "${projectName}", name);
	premakeFinal = utils::string::replaceAll(premakeFinal, "${projectType}", pType.c_str());

	const std::filesystem::path premakeFilePath = std::filesystem::path(projectDir).append("premake5.lua");
	std::ofstream fh(premakeFilePath);
	if (fh.is_open()) {
		fh << premakeFinal;
		fh.close();
	} else {
		std::cerr << "Error creating premake5.lua";
	}

	// init a git repository
	std::string gitInitCommand = utils::string::replaceAll("git init -q %s", "%s", projectDir.c_str());
	if (utils::system::runCommand(gitInitCommand) == 0) {
		std::cout << "Initialized git repository" << std::endl;
	}

	// store to repository
	Package pkg(
		std::string(name),
		projectDir.string(),
		Package::typeFromString(pType.c_str()),
		true
	);
	boost::json::array packages = Package::packagesJSON();
	
	packages.push_back(Package::toJSON(pkg));

	std::ofstream fs(Core::filePath("registry.json"));
	if (fs.is_open()) {
		fs << packages;
	}

	pkg.name = name;

	// cd to project dir
	utils::system::runCommand(utils::string::replaceAll("cd %s", "%s", projectDir.c_str()));
};

std::string Package::promptType() {
	std::cout << "Select project type:" << std::endl;
	std::cout << "1) ConsoleApp" << std::endl;
	std::cout << "2) WindowedApp" << std::endl;
	std::cout << "3) StaticLib" << std::endl;
	std::cout << "4) SharedLib" << std::endl;
	char pType;
	std::cin >> pType;

	if (pType == '1') {
		return Package::typeToString(PackageType::ConsoleApp);
	}

	if (pType == '2') {
		return Package::typeToString(PackageType::WindowedApp);
	}
	
	if (pType == '3') {
		return Package::typeToString(PackageType::StaticLib);
	}
	
	if (pType == '4') {
		return Package::typeToString(PackageType::SharedLib);
	}

	std::cerr << "Please enter a value between 1 and 4" << std::endl;

	return Package::promptType();
}

std::vector<Package> Package::packages() {
	boost::json::array packagesRaw = Package::packagesJSON();

	auto packages = std::vector<Package>();

	for (auto package: packagesRaw) {
		const auto packageData = package.as_object();
		packages.push_back(Package::fromJSON(packageData));
	}

	return packages;
}

boost::json::array Package::packagesJSON()
{
	auto registryPath = Core::filePath("registry.json");
	if (!utils::fs::pathExists(registryPath)) {
		// registry path does not exist, means no packages were ever registered or created
		return boost::json::array();
	}

	// read packages from registry
	boost::json::stream_parser parser;
	std::fstream stream(registryPath);

	if (!stream.is_open()) {
		std::cerr << "Error opening registry file" << std::endl;
		return boost::json::array();
	}

	std::string line;
	while (std::getline(stream, line)) {
		parser.write(line);
	}
	parser.finish();

	return parser.release().as_array();
}

bool Package::packageExists(const char *const name)
{
	const auto packages = Package::packagesJSON();

	for (auto package: packages) {
		if (package.as_object().at("name") == name) {
			return true;
		}
	}

	return false;
}

MaybePackageJSON Package::getJSON(const char *const name)
{
	const auto packages = Package::packagesJSON();

	for (auto package: packages) {
		if (package.as_object().at("name") == name) {
			return MaybePackageJSON(package.as_object());
		}
	}

	const char* notFound = nullptr;
	return MaybePackageJSON(notFound);
}

MaybePackage Package::get(const char *const name)
{
	const MaybePackageJSON packageJSON = Package::getJSON(name);

	if (std::holds_alternative<boost::json::object>(packageJSON)) {
		// found, return as Package instance
		return Package::fromJSON(std::get<boost::json::object>(packageJSON));
	}

	const char* notFound = nullptr;
	return MaybePackage(notFound);
}

std::string Package::typeToString(PackageType t) {
	switch (t) {
		case PackageType::ConsoleApp: return "ConsoleApp";
		case PackageType::WindowedApp: return "WindowedApp";
		case PackageType::StaticLib: return "StaticLib";
		case PackageType::SharedLib: return "SharedLib";
	}
	return "";
}

PackageType Package::typeFromString(const char *const t)
{
	if (strcmp(t, "ConsoleApp") == 0) {
		return PackageType::ConsoleApp;
	}
	if (strcmp(t, "WindowedApp") == 0) {
		return PackageType::WindowedApp;
	}
	if (strcmp(t, "StaticLib") == 0) {
		return PackageType::StaticLib;
	}
	if (strcmp(t, "SharedLib") == 0) {
		return PackageType::SharedLib;
	}

	return PackageType::ConsoleApp;
}

Package Package::fromJSON(boost::json::value data)
{
	const auto linkableObjectsRaw = data.at("linkableObjects").as_array();
	const auto dependenciesRaw = data.at("dependencies").as_array();

	std::vector<std::string> linkableObjects;
	std::vector<std::string> dependencies;

	for (auto val: linkableObjectsRaw) {
		linkableObjects.push_back(std::string(val.as_string()));
	}

	for (auto val: dependenciesRaw) {
		dependencies.push_back(std::string(val.as_string()));
	}


	return Package(
		data.at("name").as_string().c_str(),
		data.at("path").as_string().c_str(),
		Package::typeFromString(data.at("type").as_string().c_str()),
		data.at("version").as_string().c_str(),
		linkableObjects,
		dependencies,
		data.at("managed").as_bool(),
		data.at("registeredAt").as_int64()
	);
}

boost::json::object Package::toJSON(Package &pkg)
{
	boost::json::array linkable;

	for (auto item: pkg.linkableObjects) {
		linkable.push_back(boost::json::string(item));
	}

	boost::json::array dependencies;

	for (auto item: pkg.dependencies) {
		dependencies.push_back(boost::json::string(item));
	}

	boost::json::object json;
	json["name"] = pkg.name;
	json["path"] = pkg.path;
	json["type"] = Package::typeToString(pkg.type);
	json["version"] = pkg.version;
	json["managed"] = pkg.managed;
	json["registeredAt"] = pkg.registeredAt;
	json["linkableObjects"] = linkable;
	json["dependencies"] = dependencies;
	return json;
}