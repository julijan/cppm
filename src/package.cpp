#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>

#include "package.h"
#include "utils.h"
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
	std::string pType = Package::promptType(false);

	// init a git repository
	std::string gitInitCommand = utils::string::replaceAll("git init -q %s", "%s", projectDir.c_str());
	if (utils::system::runCommand(gitInitCommand) == 0) {
		std::cout << "Initialized git repository" << std::endl;
	}

	// create instance
	Package pkg(
		std::string(name),
		projectDir.string(),
		Package::typeFromString(pType.c_str()),
		true
	);

	// store to registry
	Package::addToRegistry(pkg);

	// create premake5.lua
	Package::generatePremake(pkg);

	// cd to project dir
	utils::system::runCommand(utils::string::replaceAll("cd %s", "%s", projectDir.c_str()));
}

void Package::registerPackage(const std::filesystem::path& p) {
	// make sure a package in this path is not already registered
	MaybePackage existing = Package::includesPath(p);
	
	if (std::holds_alternative<Package>(existing)) {
		Package existingPkg = std::get<Package>(existing);
		std::cerr << existingPkg.name << " already registered at path " << existingPkg.path << std::endl;
		return;
	}

	// no existing package in the path, ok to register
	// assume package name = current directory name
	// if package with such name exists, append it with attempt number
	std::string assumedName = p.filename().string();

	if (Package::packageExists(assumedName.c_str())) {
		int attempt = 1;
		while (true) {
			std::string attemptedName = assumedName + std::to_string(attempt);
			if (!Package::packageExists(attemptedName.c_str())) {
				// found an available name
				assumedName = attemptedName;
				break;
			}
			attempt++;
		}
	}

	std::cout << "Registering current path as a package with name " << assumedName << std::endl;
	std::cout << "If you want to use a different name please enter it bellow and press enter, leave blank to use " << assumedName << std::endl;

	// prompt for alternative name
	std::string nameAlternative;
	std::getline(std::cin, nameAlternative, '\n');

	std::string nameFinal;

	if (utils::string::trim(nameAlternative).length() > 0) {
		// alternative name entered
		nameFinal = utils::string::trim(nameAlternative);
	} else {
		nameFinal = assumedName;
	}

	PackageType pType = Package::typeFromString(Package::promptType(true).c_str());

	Package pkg(
		nameFinal,
		p,
		pType,
		false
	);

	// register the package
	Package::addToRegistry(pkg);

	std::cout << "Package " << pkg.name << " registered" << std::endl;
}

std::string Package::promptType(bool expectLibrary) {
	// if expectLibrary (only for non-managed projects), list is limited to library types
	std::initializer_list<const char*> options = expectLibrary ?
		std::initializer_list({ "StaticLib", "SharedLib" }) :
		std::initializer_list({ "ConsoleApp", "WindowedApp", "StaticLib", "SharedLib" });

	
	// prompt user to select type
	std::cout << "Select project type:" << std::endl;
	for (int i = 0; i < options.size(); i++) {
		auto option = *(options.begin()+i);
		std::cout << (i+1) << ") " << option << std::endl;
	}

	char pType;
	std::cin >> pType;

	// cast user input to int
	int optionIndex = pType - '0' - 1;

	if (optionIndex < 0 || optionIndex > options.size() - 1) {
		// invalid user input
		std::cerr << "Please enter a value between 1 and 4" << std::endl;
		return Package::promptType(expectLibrary);
	}

	return *(options.begin() + optionIndex);
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

std::vector<Package> Package::filter(std::function<bool(Package&)> predicate)
{
	std::vector<Package> packages = Package::packages();

	std::vector<Package> matched;
	std::copy_if(
		packages.begin(),
		packages.end(),
		std::back_inserter(matched),
		predicate
	);

	return matched;
}

MaybePackage Package::find(std::function<bool(Package&)> predicate)
{
	std::vector<Package> packages = Package::packages();
	
	auto found = std::find_if(packages.begin(), packages.end(), predicate);

	if (found != packages.end()) {
		// not found
		return *found;
	}
	
	return PackageNotFound();
}

MaybePackageJSON Package::getJSON(const char *const name)
{
	const auto packages = Package::packagesJSON();

	for (auto package: packages) {
		if (package.as_object().at("name") == name) {
			return MaybePackageJSON(package.as_object());
		}
	}

	return PackageNotFound();
}

std::vector<std::string> Package::dependencyNames(const char *const name)
{
	MaybePackage package = Package::get(name);

	if (std::holds_alternative<Package>(package)) {
		// package exists, return dependencies
		std::vector<std::string> dependencyNames = std::get<Package>(package).dependencies;

		// make sure dependencies exist, remove non-existent
		auto end = std::remove_if(dependencyNames.begin(), dependencyNames.end(), [](std::string depName) {
			return !Package::packageExists(depName.c_str());
		});
		dependencyNames.erase(end, dependencyNames.end());

		return dependencyNames;
	}

	return std::vector<std::string>();
}

std::vector<Package> Package::getDependencies(const char *const name)
{
	MaybePackage package = Package::get(name);

	if (std::holds_alternative<Package>(package)) {
		// package exists, return dependencies
		std::vector<std::string> dependencyNames = Package::dependencyNames(name);

		// return the dependencies as Package instance
		std::vector<Package> dependencies;
		std::transform(
			dependencyNames.begin(),
			dependencyNames.end(),
			std::back_inserter(dependencies),
			[](std::string depName) {
				return std::get<Package>(Package::get(depName.c_str()));
			}
		);

		return dependencies;
	}

	return std::vector<Package>();
}

void Package::addDependency(Package &pkg, Package &dep)
{
	if (Package::isDependency(pkg, dep)) {
		std::cerr << dep.name << " already a dependecy of " << pkg.name << std::endl;
		return;
	}


	// add dependency and update registry
	pkg.dependencies.push_back(dep.name);
	Package::updateRegistry(pkg);

	// create symbolic link in /includes
	std::filesystem::path pkgPath = pkg.path;
	std::filesystem::path includesPath = std::filesystem::path(pkgPath).append("includes");
	std::filesystem::path symlinkPath = std::filesystem::path(includesPath).append(dep.name);

	std::filesystem::path symlinkTarget = dep.path;
	
	std::filesystem::create_directory_symlink(symlinkTarget, symlinkPath);
}

void Package::addDependency(Package &pkg, const char *const name)
{
	MaybePackage dependency = Package::get(name);
	if (std::holds_alternative<PackageNotFound>(dependency)) {
		std::cerr << "Package " << name << " does not exist" << std::endl;
		return;
	}

	Package::addDependency(pkg, std::get<Package>(dependency));
}

void Package::removeDependency(Package &pkg, Package &dep)
{
	if (!Package::isDependency(pkg, dep)) {
		std::cerr << dep.name << " is not a dependency of " << pkg.name << std::endl;
		return;
	}

	// remove dependency
	auto end = std::remove_if(
		pkg.dependencies.begin(),
		pkg.dependencies.end(),
		[&dep](std::string& depName) {
			return depName == dep.name;
		}
	);
	pkg.dependencies.erase(end, pkg.dependencies.end());

	// remove symlink
	std::filesystem::path symlinkPath = std::filesystem::path(pkg.path).append("includes").append(dep.name);
	if (std::filesystem::exists(symlinkPath)) {
		std::filesystem::remove(symlinkPath);
	}

	// store to registry without dependency
	Package::updateRegistry(pkg);

	std::cout << "Dependency " << dep.name << " removed" << std::endl;
}

void Package::removeDependency(Package &pkg, const char *const name)
{
	MaybePackage dependency = Package::get(name);

	if (std::holds_alternative<PackageNotFound>(dependency)) {
		std::cerr << "Can't remove non-existent dependency " << name << std::endl;
		return;
	}

	Package::removeDependency(pkg, std::get<Package>(dependency));
}

bool Package::isDependency(Package &pkg, const char *const depName)
{
	auto it = std::find_if(pkg.dependencies.begin(), pkg.dependencies.end(), [depName](std::string existingDep) {
		return existingDep == depName;
	});

	return it != pkg.dependencies.end();
}

bool Package::isDependency(Package &pkg, Package &dep)
{
	return Package::isDependency(pkg, dep.name.c_str());
}

bool Package::isDependency(const char *const pkgName, const char *const depName)
{
	MaybePackage pkg = Package::get(pkgName);
	if (std::holds_alternative<PackageNotFound>(pkg)) {
		return false;
	}

	return Package::isDependency(std::get<Package>(pkg), depName);
}

bool Package::isLibrary(Package &pkg)
{
	return pkg.type == PackageType::StaticLib || pkg.type == PackageType::SharedLib;
}

void Package::generatePremake(Package &pkg)
{
	std::filesystem::path premakePath = std::filesystem::path(pkg.path).append("premake5.lua");
	std::ofstream fstream(premakePath);

	if (!fstream.is_open()) {
		std::cerr << "Could not open premake file for writing, path: " << premakePath.string() << std::endl;
		return;
	}

	// workspace
	fstream << "workspace \"" << pkg.name << '"' << std::endl; //variable
	fstream << "\tconfigurations { \"Debug\", \"Release\" }" << std::endl << std::endl;

	// project
	fstream << "project \"" << pkg.name << '"' << std::endl; // variable
	fstream << "\tlanguage \"C++\"" << std::endl;
	fstream << "\tkind \"" << Package::typeToString(pkg.type) << '"' << std::endl; // variable
	fstream << "\tcppdialect \"C++20\"" << std::endl;
	fstream << "\tarchitecture \"x64\"" << std::endl;
	fstream << "\ttargetdir \"bin/%{cfg.buildcfg}\"" << std::endl;
	fstream << "\tfiles { \"./src/**.h\", \"./src/**.cpp\" }" << std::endl;
	fstream << "\tincludedirs { \"./includes\" }" << std::endl;

	if (pkg.dependencies.size() > 0) {
		// link libraries

		// collect all linked objects in a vector
		std::vector<std::string> linked;

		for (std::string& depName: pkg.dependencies) {
			MaybePackage dependency = Package::get(depName.c_str());
			if (std::holds_alternative<PackageNotFound>(dependency)) {
				// missing dependency!
				std::cerr << "Missing dependency " << depName << ", resuming" << std::endl;
				continue;
			}

			Package dependencyPkg = std::get<Package>(dependency);

			if (!Package::isLibrary(dependencyPkg)) {
				// not a library
				// what is it then? Probably should not be a dependency in the first place
				// skip
				continue;
			}
			
			if (dependencyPkg.linkableObjects.size() > 0) {
				// has linkable objects, include them
				for (std::string link: dependencyPkg.linkableObjects) {
					linked.push_back(link);
				}
			} else {
				// no linkable objects, this could mean the registry is out of date or library was never built
				// assume package name
				linked.push_back(dependencyPkg.name);
			}
		}

		if (linked.size() > 0) {
			// include links in premake
			fstream << "\tlinks {" << std::endl;

			for (std::string& link: linked) {
				fstream << "\t\t\"" << link  << '"' << std::endl;
			}

			fstream << "\t}";
		}
	}

	// filters
	fstream << "\tfilter \"configurations:Debug\"" << std::endl;
	fstream << "\t\tdefines { \"DEBUG\" }" << std::endl;
	fstream << "\t\tsymbols \"On\"" << std::endl;

	fstream << "\tfilter \"configurations:Release\"" << std::endl;
	fstream << "\t\tdefines { \"NDEBUG\" }" << std::endl;
	fstream << "\t\toptimize \"On\"" << std::endl;

	// project test
	fstream << '\n';
	fstream << "project \"" << pkg.name << "Test\"" << std::endl; // variable
	fstream << "\tlanguage \"C++\"" << std::endl;
	fstream << "\tkind \"ConsoleApp\"" << std::endl;
	fstream << "\tcppdialect \"C++20\"" << std::endl;
	fstream << "\tarchitecture \"x64\"" << std::endl;
	fstream << "\ttargetdir \"tests/%{cfg.buildcfg}\"" << std::endl;
	fstream << "\tfiles { \"./src/test.cpp\" }" << std::endl;
	fstream << "\tincludedirs { \"./src\", \"./includes\" }" << std::endl;

	if (Package::isLibrary(pkg)) {
		// link package itself to test, if pkg is a library
		fstream << "\tlinks {" << std::endl;
		
		if (pkg.linkableObjects.size() > 0) {
			for (std::string link: pkg.linkableObjects) {
				fstream << "\t\t\"" << link << '"';
			}
			fstream << "\t}" << std::endl;
		} else {
			// pkg is a library with no linkableObjects
			// assume pkg.name
			fstream << " \"" << pkg.name << " \"" << std::endl;
		}

	}

	// filters
	fstream << "\tfilter \"configurations:Debug\"" << std::endl;
	fstream << "\t\tdefines { \"DEBUG\" }" << std::endl;
	fstream << "\t\tsymbols \"On\"" << std::endl;

	fstream << "\tfilter \"configurations:Release\"" << std::endl;
	fstream << "\t\tdefines { \"NDEBUG\" }" << std::endl;
	fstream << "\t\toptimize \"On\"" << std::endl;

	// done, flush and close
	fstream.flush();
	fstream.close();
}

MaybePackage Package::includesPath(std::filesystem::path p)
{
	const std::string pathString = p.string();
	return Package::find([&pathString](Package& pkg) {
		return pathString.starts_with(pkg.path);
	});
}

MaybePackage Package::inPath(std::filesystem::path p)
{
	const std::string pathString = p.string();
	return Package::find([&pathString](Package& pkg) {
		return pkg.path == pathString;
	});
}

MaybePackage Package::get(const char *const name)
{
	return Package::find([name](Package pkg) {
		return pkg.name == name;
	});
}

void Package::writeRegistry(const boost::json::value &json)
{
	std::ofstream fh(Core::filePath("registry.json"));
	if (!fh.is_open()) {
		std::cerr << "Error initializing package registry" << std::endl;
		return;
	}
	
	fh << json;
}

void Package::initRegistry() {
	boost::json::array registry = {};
	Package::writeRegistry(registry);
}

void Package::addToRegistry(Package& pkg)
{
	boost::json::array packages = Package::packagesJSON();
	packages.push_back(Package::toJSON(pkg));

	Package::writeRegistry(packages);
}

void Package::updateRegistry(Package &pkg)
{
	auto packages = Package::packagesJSON();

	unsigned int index = 0;
	for (auto package: packages) {
		if (package.at("name").as_string() == pkg.name) {
			// found the package to be updated, replace
			packages[index] = Package::toJSON(pkg);
			Package::writeRegistry(packages);
			return;
		}
		index++;
	}

	// if we got here it means package was not found in the registry
	std::cerr << "Can't update package registry, package " << pkg.name << " not registered" << std::endl;
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

void Package::listDependencies(Package &pkg)
{
	if (pkg.dependencies.size() == 0) {
		std::cout << "No dependencies" << std::endl;
	} else {
		std::cout << "+ Dependencies:" << std::endl;
		for (std::string& depName: pkg.dependencies) {
			std::cout << "|- " << depName << std::endl;
		}
	}
}

void Package::display(Package& pkg)
{
	std::cout << "Package name: " << pkg.name << std::endl;
	std::cout << "Version: " << pkg.version << std::endl;
	std::cout << "Type: " << Package::typeToString(pkg.type) << std::endl;
	std::cout << "Path: " << pkg.path << std::endl;

	Package::listDependencies(pkg);

	if ((pkg.type == PackageType::StaticLib || pkg.type == PackageType::SharedLib) && pkg.linkableObjects.size() > 0) {
		// show linkable objects
		std::cout << "Linkable objects:" << std::endl;
		for (std::string& objName: pkg.linkableObjects) {
			std::cout << "|- " << objName << std::endl;
		}
	}
}

void Package::display(const char *const name)
{
	MaybePackage pkg = Package::get(name);

	if (std::holds_alternative<PackageNotFound>(pkg)) {
		std::cout << "Package " << name << " does not exist";
		return;
	}

	Package::display(std::get<Package>(pkg));
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

Package Package::fromJSON(PackageJSON data)
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

PackageJSON Package::toJSON(Package &pkg)
{
	boost::json::array linkable;

	for (auto item: pkg.linkableObjects) {
		linkable.push_back(boost::json::string(item));
	}

	boost::json::array dependencies;

	for (auto item: pkg.dependencies) {
		dependencies.push_back(boost::json::string(item));
	}

	PackageJSON json;
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