#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>

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
		std::filesystem::create_directory(projectDir);
	} catch(std::filesystem::filesystem_error e) {
		std::cerr << "Error creating project directory: " << e.what() << std::endl;
	}

	// create sub-directories
	std::filesystem::create_directory(utils::fs::extendPath<1>(projectDir, { "src" }));
	Package::createIncludesDirectories(projectDir);

	// select project type
	std::string pType = Package::promptType(false);

	// create instance
	Package pkg(
		std::string(name),
		projectDir.string(),
		Package::typeFromString(pType.c_str()),
		true
	);

	// init a git repository
	std::string gitInitCommand = utils::string::replaceAll("git init -q %s", "%s", projectDir.c_str());
	if (utils::system::runCommand(gitInitCommand) == 0) {
		std::cout << "Initialized git repository" << std::endl;
	}

	// create git pre-push hook
	const auto preHookPath = Package::getPath<3>(pkg, { ".git", "hooks", "pre-push" });
	std::ofstream fsGitHook(preHookPath);
	if (fsGitHook.is_open()) {
		fsGitHook << "#!/bin/bash" << std::endl;
		fsGitHook << "if [ \"$CPPM_ENABLE_GIT\" != \"1\" ]; then" << std::endl;
		fsGitHook << "    echo \"You may want to consider using 'cppm push' instead of 'git push'\"" << std::endl;
		fsGitHook << "    echo \"To understand why, please run 'cppm help push'\"" << std::endl;
		fsGitHook << "    echo \"If you want to use git push anyway, set env variable CPPM_ENABLE_GIT=1 (export CPPM_ENABLE_GIT=1)\"" << std::endl;
		fsGitHook << "    exit 1" << std::endl;
		fsGitHook << "fi\n" << std::endl;
		fsGitHook << "exit 0" << std::endl;

		fsGitHook.close();
	}

	// make it executable
	const auto preHookPermissions =
		std::filesystem::perms::owner_all |
		std::filesystem::perms::group_all |
		std::filesystem::perms::others_exec;
	std::filesystem::permissions(preHookPath, preHookPermissions);


	// store to registry
	Package::addToRegistry(pkg);

	// create hello world entry point
	std::string helloWorldFileName = pkg.name + ".cpp";
	std::ofstream fs(std::filesystem::path(projectDir).append("src").append(helloWorldFileName));

	if (fs.is_open()) {
		// no need to fail here if failed to open
		// hello world is not required, just a convenience
		fs << "#include <iostream>\n" << std::endl;
		fs << "int main() {" << std::endl;
		fs << "\tstd::cout << \"Hello, world!\" << std::endl;" << std::endl;
		fs << "\treturn 0;" << std::endl;
		fs << '}' << std::endl;

		fs.close();
	}

	// create .gitignore
	std::filesystem::path gitignorePath = Package::getPath<1>(pkg, { ".gitignore" });
	std::ofstream fsGitIgnore(gitignorePath);
	fsGitIgnore << "includes" << std::endl;
	fsGitIgnore.close();

	// create README.md
	std::filesystem::path readmePath = Package::getPath<1>(pkg, { "README.md" });
	std::ofstream fsReadme(readmePath);
	fsReadme << R"(Don't write config files, write C++!

Before you start programming, a brief description of the file structure created:
The following is created:
+-src (all your code goes here)
|--projectName.cpp (entry point for your application with a simple "Hello World" application)
+-includes (all dependencies you include will be placed here, never create files here!)
|-premake5.lua (this file is used to describe your project to the compiler)
|-.gitignore (includes directories ignored, this does not mean they will not be pushed to remote, to learn about this run cppm help push)
|-README.md (this file, feel free to edit or delete it)

This means that you will only ever need to edit files within ./src directory. You can create directories within src if you need to structure your code in that way.

To create new source files run `cppm add [srcName]`
To include another package run `cppm include [packageName]`
To compile your project run `cppm build`

Package? In cppm dialect, a package is what is more commonly referred to as project, it can be an application or a library. Package can be a managed package (your project, just like this one) or non-managed (3rd party code). You can register non-managed packages using `cppm register`, run `cppm help register` to learn how to use the command.

You can easily register packages you installed using vcpkg by running `cppm vcpkg-register [packageName]`, then you can include them in your package using `cppm include [packageName]`

Lastly, a git repository has been initialized for you, if you don't use git, delete .git directory.

run `cppm help` to learn about all the features at your disposal.
	)" << std::endl;
	fsReadme.close();

	// create premake5.lua
	Package::generatePremake(pkg);
}

// create includes/lib and includes/src
// includes/src contains symlinks to dependency -> ./src
// includes/lib contains symlinks to dependency -> ./bin
void Package::createIncludesDirectories(const Package& pkg)
{
	const std::filesystem::path path = Package::getPath(pkg);
	Package::createIncludesDirectories(path);
}

void Package::createIncludesDirectories(const std::filesystem::path& p)
{
	std::filesystem::path includesDir = utils::fs::extendPath<1>(p, { "includes" });
	std::filesystem::path includesSrc = utils::fs::extendPath<2>(p, { "includes", "src" });
	std::filesystem::path includesLib = utils::fs::extendPath<2>(p, { "includes", "lib" });

	// re-create includes directories
	if (!std::filesystem::exists(includesDir)) {
		std::filesystem::create_directory(includesDir);
	}
	if (!std::filesystem::exists(includesSrc)) {
		std::filesystem::create_directory(includesSrc);
	}
	if (!std::filesystem::exists(includesLib)) {
		std::filesystem::create_directory(includesLib);
	}
}

template <int Depth>
std::filesystem::path Package::getPath(const char *const pkgName, SmartArray<const char*, Depth> subdirs)
{
	MaybePackage pkg = Package::get(pkgName);

	if (std::holds_alternative<PackageNotFound>(pkg)) {
		// Package not found! return /dev/null which seems like the safest option
		return std::filesystem::path("/dev/null");
	}

	return Package::getPath(std::get<Package>(pkg), subdirs);
}

template <int Depth>
std::filesystem::path Package::getPath(const Package& pkg, SmartArray<const char*, Depth> subdirs)
{
	std::filesystem::path p(pkg.path);
	return utils::fs::extendPath<Depth>(p, subdirs);
}

std::filesystem::path Package::getPath(const Package &pkg)
{
	return Package::getPath<0>(pkg, {});
}

void Package::move(Package &pkg, const std::filesystem::path toBare)
{
	const std::filesystem::path currentPath  = Package::getPath(pkg);
	const auto currentDirname = currentPath.filename();

	// include package name in to so user doesn't have to enter it when entering the to path
	std::filesystem::path to = utils::fs::extendPath<1>(toBare, { currentDirname.c_str() });


	// make sure the destination is not a package directory
	// can't move a package into another package
	MaybePackage packageInDestination = Package::inPath(to);

	if (std::holds_alternative<Package>(packageInDestination)) {
		std::cerr << "Can't move package to " << to << " package " << std::get<Package>(packageInDestination).name << " there" << std::endl;
		return;
	}

	// get dependents
	const std::vector<Package> dependents = Package::dependents(pkg.name.c_str());

	// move package
	std::filesystem::rename(currentPath, to);

	// update in registry
	pkg.path = to.string();
	Package::updateRegistry(pkg);

	// update dependents
	int depsUpdated = 0;
	for (const Package& dependent: dependents) {
		// create new symlinks
		Package::linkDependency(dependent, pkg);

		// keep track how many dependents were updated
		depsUpdated++;
	}

	std::cout << "Package " << pkg.name << " moved to " << to << "." << std::endl;
	if (depsUpdated > 0) {
		std::cout << "Updated " << depsUpdated << " dependent packages" << std::endl;
	}
}

void Package::registerPackage(const std::filesystem::path &p, bool managed, const char* assumeName)
{
	// make sure a package in this path is not already registered
	MaybePackage existing = Package::includesPath(p);
	
	if (std::holds_alternative<Package>(existing)) {
		Package existingPkg = std::get<Package>(existing);
		std::cerr << existingPkg.name << " already registered at path " << existingPkg.path << std::endl;
		return;
	}

	// if registering as managed package, make sure the package is valid
	if (managed && !Package::checkPath(p, true, false)) {
		// attempted to register directory as a managed package
		// directory is not package-like
		// offer to interactively fix package
		std::cout << "Current directory does not conform to managed package structure. Do you want to interactively fix it so you can proceed with the operation? (Y)Yes / (N)No" << std::endl;
		char action;
		while (true) {
			std::cin >> action;
			if (action == 'N' || action == 'n') {
				// user canceled
				return;
			}

			if (action == 'Y' || action == 'y') {
				// proceed
				break;
			}
		}
		Package::fixPath(p);

		if (!Package::checkPath(p, true, false)) {
			// still does not conform
			std::cout << "Directory still does not conform, if you approved all fixes and you see this, report an issue" << std::endl;
			return;
		}

		// if we are here, user managed to make the directory conform to managed packaege structure
	}

	// no existing package in the path, ok to register
	// assume package name = current directory name
	// if package with such name exists, append it with attempt number
	std::string assumedName = assumeName == nullptr ? p.filename().string() : assumeName;

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

	std::cout << "Registering current path as a " << (managed ? "managed" : "non-managed") << " package with name " << assumedName << std::endl;
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

	PackageType pType = Package::typeFromString(Package::promptType(!managed).c_str());

	Package pkg(
		nameFinal,
		p,
		pType,
		managed
	);

	if (!managed) {
		// non managed package, we must discover all linkable objects
		const auto linkable = Package::findLinkableObjects(p);
		pkg.linkableObjects.insert(pkg.linkableObjects.end(), linkable.begin(), linkable.end());
	}

	// register the package
	Package::addToRegistry(pkg);

	std::cout << "Package " << pkg.name << " registered" << std::endl;
}

void Package::unregisterPackage(const char *const name)
{

	MaybePackage pkg = Package::get(name);

	if (std::holds_alternative<PackageNotFound>(pkg)) {
		std::cerr << "Package " << name << " not registered";
		return;
	}

	if (std::get<Package>(pkg).managed) {
		// confirm unregistering of managed packages
		std::cout << "You are about to unregister a managed (your own) package " << name << std::endl;
		std::cout << "Proceed? (Y)Yes / (N)No" << std::endl;
		char action;
		while (true) {
			std::cin >> action;

			if (action == 'Y' || action == 'y') {
				// proceed
				break;
			}

			if (action == 'N' || action == 'n') {
				// abort
				return;
			}
		}
	}

	std::vector<Package> dependents = Package::dependents(name);

	if (dependents.size() > 0) {
		std::cout << dependents.size() << " packages depend on " << name << std::endl;
		std::cout << "If you unregister this package, it will be removed from dependency list of it's dependents, as a result, affected packages may not work as expected" << std::endl;
		std::cout << "What do you want to do? (C)Cancel / (U)Unregister:" << std::endl;

		char action;
		while (true) {
			std::cin >> action;
			if (action == 'C' || action == 'c') {
				// user decided to cancel the operation
				return;
			}

			if (action == 'U' || action == 'u') {
				// user decided to proceed with unregistering
				break;
			}
		}

		// remove as dependency for all dependents
		for (Package& dependent: dependents) {
			Package::removeDependency(dependent, std::get<Package>(pkg));
		}
	}

	Package::removeFromRegistry(std::get<Package>(pkg));

	std::cout << "Package " << name << " unregistered" << std::endl;
}

std::vector<std::filesystem::path> Package::findLinkableObjects(const std::filesystem::path &p)
{
	std::vector<std::filesystem::path> linkable;
	const auto pIter = std::filesystem::directory_iterator(p);
	for (const auto f: pIter) {
		if (f.is_directory()) {
			// directory, resume recursively
			auto linkableInDir = Package::findLinkableObjects(f);
			linkable.insert(linkable.end(), linkableInDir.begin(), linkableInDir.end());
		} else {
			// file, check extension
			if (f.path().string().ends_with(".a")) {
				// found linkable object
				linkable.push_back(f.path());
			}
		}
	}

	return linkable;
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

Maybe<std::filesystem::path> Package::vcpkgPackagePath(std::string &pkgName)
{
	std::string vcpkgDir = utils::system::appDataDir() + std::filesystem::path::preferred_separator + ".vcpkg";
	std::filesystem::path pkgPath = utils::fs::extendPath<1>(vcpkgDir, { "packages" });
	const auto iter = std::filesystem::directory_iterator(pkgPath);
	for (auto dir: iter) {
		if (dir.is_directory() && dir.path().filename().string().starts_with(pkgName + "_")) {
			return dir.path();
		}
	}
	return Empty();
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
	return utils::json::read(registryPath).as_array();
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
		return Package::getDependencies(std::get<Package>(package));
	}

	return std::vector<Package>();
}

std::vector<Package> Package::getDependencies(const Package &pkg)
{
	// return the dependencies as Package instance
	std::vector<Package> dependencies;
	std::transform(
		pkg.dependencies.begin(),
		pkg.dependencies.end(),
		std::back_inserter(dependencies),
		[](std::string depName) {
			return std::get<Package>(Package::get(depName.c_str()));
		}
	);

	// sort, managed first
	std::sort(
		dependencies.begin(),
		dependencies.end(),
		[](auto a, auto b) {
			int aVal = a.managed ? 1 : 0;
			int bVal = b.managed ? 1 : 0;
			return aVal > bVal;
		}
	);

	return dependencies;
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

	if (pkg.managed) {
		// create symbolic links
		Package::linkDependency(pkg, dep);
	
		// re-generate premake
		Package::generatePremake(pkg);
	}

	// no longer needed
	// linkDependencies will link all transient dependencies in includes
	// so they no longer need to be a direct dependency
	// if (pkg.managed && !dep.managed) {
	// 	// adding a non-managed dependency to a managed package
	// 	// all transient dependencies must be added too
	// 	for (std::string& transient: dep.dependencies) {
	// 		Package::addDependency(pkg, transient.c_str());
	// 	}
	// }
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

	// remove symlinks
	Package::unlinkDependency(pkg, dep);

	// store to registry without dependency
	Package::updateRegistry(pkg);

	// re-generate premake
	Package::generatePremake(pkg);

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

void Package::linkDependency(const Package &pkg, const Package &dep)
{
	// remove existing symlinks if they exist
	Package::unlinkDependency(pkg, dep);

	// create symbolic link in /includes/src, used by includedirs
	const std::filesystem::path includesSymlinkPath = Package::getPath<3>(
		pkg, { "includes", "src", dep.name.c_str() }
	);
	std::filesystem::path includesSymlinkTarget = Package::dependencyTargetIncludes(dep);

	std::filesystem::create_directory_symlink(includesSymlinkTarget, includesSymlinkPath);

	// create symbolic link in /includes/lib, used by libdirs
	std::filesystem::path libsSymlinkPath = Package::getPath<3>(
		pkg, { "includes", "lib", dep.name.c_str() }
	);
	std::filesystem::path libsSymlinkTarget = Package::dependencyTargetLib(dep);

	std::filesystem::create_directory_symlink(libsSymlinkTarget, libsSymlinkPath);

	// link transient dependencies recursively
	if (dep.dependencies.size() > 0) {
		for (const std::string& depName: dep.dependencies) {
			MaybePackage dep = Package::get(depName.c_str());
			if (std::holds_alternative<PackageNotFound>(dep)) {
				std::cerr << "Skipped linking a missing dependency " << depName << std::endl;
				continue;
			}

			Package::linkDependency(pkg, std::get<Package>(dep));
		}
	}
}

void Package::linkDependencies(const Package &pkg)
{
	const std::vector<Package> dependencies = Package::getDependencies(pkg);
	for (const Package& dep: dependencies) {
		// create symlinks
		Package::linkDependency(pkg, dep);
	}
}

void Package::unlinkDependency(const Package &pkg, const Package &dep)
{
	Package::unlinkDependency(pkg, dep.name.c_str());
}

void Package::unlinkDependency(const Package &pkg, const char *depName)
{
	MaybePackage depMaybe = Package::get(depName);

	if (std::holds_alternative<PackageNotFound>(depMaybe)) {
		std::cerr << "Attempted to unlink a non-existent dependency" << std::endl;
		return;
	}

	Package dep = std::get<Package>(depMaybe);

	const auto srcLink = Package::getPath<3>(pkg, { "includes", "src", depName });
	const auto libLink = Package::getPath<3>(pkg, { "includes", "lib", depName });
	if (std::filesystem::exists(srcLink)) {
		std::filesystem::remove(srcLink);
	}
	if (std::filesystem::exists(libLink)) {
		std::filesystem::remove(libLink);
	}

	// unlink transient dependencies recursively
	if (dep.dependencies.size() > 0) {
		for (const std::string& depName: dep.dependencies) {
			MaybePackage dep = Package::get(depName.c_str());
			if (std::holds_alternative<PackageNotFound>(dep)) {
				std::cerr << "Skipped unlinking a missing dependency " << depName << std::endl;
				continue;
			}

			if (!Package::isTransientDependency(pkg, std::get<Package>(dep))) {
				Package::unlinkDependency(pkg, std::get<Package>(dep));
			}
		}
	}
}

std::filesystem::path Package::dependencyTargetIncludes(const Package &dep)
{
	std::filesystem::path pkgPath(Package::getPath(dep));
	if (dep.managed) {
		// use package's src dir
		return utils::fs::extendPath<1>(pkgPath, { "src" });
	}

	// we don't know anything about non managed packages, so we always link entire package dir
	return pkgPath;
}

std::filesystem::path Package::dependencyTargetLib(const Package &dep)
{
	std::filesystem::path pkgPath(Package::getPath(dep));
	if (dep.managed) {
		// use package's bin dir
		return utils::fs::extendPath<1>(pkgPath, { "bin" });
	}

	// we don't know anything about non managed packages, so we always link entire package dir
	return pkgPath;
}

bool Package::isDependency(const Package &pkg, const char *const depName)
{
	auto it = std::find_if(pkg.dependencies.begin(), pkg.dependencies.end(), [depName](std::string existingDep) {
		return existingDep == depName;
	});

	return it != pkg.dependencies.end();
}

bool Package::isDependency(const Package &pkg, const Package &dep)
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

bool Package::isTransientDependency(const Package& pkg, const Package& dep)
{
	bool direct = Package::isDependency(pkg, dep);

	if (direct) {return true;}

	// check if transient of some
	const auto it = std::find_if(
		pkg.dependencies.begin(),
		pkg.dependencies.end(),
		[&dep](const std::string& depName) {
			MaybePackage pkgDep = Package::get(depName.c_str());
			if (std::holds_alternative<PackageNotFound>(pkgDep)) {
				std::cerr << "Package missing in chain " << depName << std::endl;
				return false;
			}

			return Package::isTransientDependency(std::get<Package>(pkgDep), dep);
		}
	);

	return it != pkg.dependencies.end();
}

bool Package::isTransientDependency(const Package &pkg, const char *depName)
{
	MaybePackage depMaybe = Package::get(depName);
	if (std::holds_alternative<PackageNotFound>(depMaybe)) {
		std::cerr << "Missing dependency in the chanin " << depName << std::endl;
		return false;
	}
	return Package::isTransientDependency(pkg, std::get<Package>(depMaybe));
}

void Package::materializeDependencies(const Package &pkg)
{

	if (!pkg.managed || pkg.dependencies.size() == 0) {
		// nothing to do
		return;
	}

	// delete entire includes directory
	std::filesystem::path includesDir = Package::getPath<1>(pkg, { "includes" });
	std::filesystem::path includesSrc = Package::getPath<2>(pkg, { "includes", "src" });
	std::filesystem::path includesLib = Package::getPath<2>(pkg, { "includes", "lib" });
	std::filesystem::remove_all(includesDir);

	// re-create includes directories
	Package::createIncludesDirectories(pkg);

	for (const std::string& depName: pkg.dependencies) {
		MaybePackage depMaybe = Package::get(depName.c_str());
		if (std::holds_alternative<PackageNotFound>(depMaybe)) {
			std::cerr << "Dependency " << depName << " not found! Skipped." << std::endl;
			continue;
		}

		// dependency exists
		const Package& dep = std::get<Package>(depMaybe);

		// materialize dependency before copying contents
		Package::materializeDependencies(dep);

		// copy contents of the dependency to includes
		const std::filesystem::path depSrc = Package::dependencyTargetIncludes(dep);
		const std::filesystem::path depLib = Package::dependencyTargetLib(dep);
		std::filesystem::copy(
			depSrc,
			utils::fs::extendPath<1>(includesSrc, { depName.c_str() }),
			std::filesystem::copy_options::recursive
		);
		std::filesystem::copy(
			depLib,
			utils::fs::extendPath<1>(includesLib, { depName.c_str() }),
			std::filesystem::copy_options::recursive
		);

		// un-materialize dependency
		Package::unmaterializeDependencies(dep);
	}

	std::cout << "Package " << pkg.name << " dependencies materialized" << std::endl;
}

void Package::unmaterializeDependencies(const Package &pkg)
{
	if (!pkg.managed || pkg.dependencies.size() == 0) {
		// nothing to do
		return;
	}

	// delete entire includes directory
	std::filesystem::path includesDir = Package::getPath<1>(pkg, { "includes" });
	std::filesystem::path includesSrc = Package::getPath<2>(pkg, { "includes", "src" });
	std::filesystem::path includesLib = Package::getPath<2>(pkg, { "includes", "lib" });
	std::filesystem::remove_all(includesDir);

	// re-create includes directories
	Package::createIncludesDirectories(pkg);

	// create dependency symlinks
	Package::linkDependencies(pkg);

	// un-materialize recursively
	for (const std::string depName: pkg.dependencies) {
		MaybePackage depMaybe = Package::get(depName.c_str());
		if (std::holds_alternative<Package>(depMaybe)) {
			Package::unmaterializeDependencies(std::get<Package>(depMaybe));
			continue;
		}
		std::cerr << "Can't unmaterialize dependency of " << pkg.name << ", " << depName << ", not found. Skipped." << std::endl;
	}

	std::cout << "Package " << pkg.name << " dependencies unmaterialized" << std::endl;
}

std::vector<Package> Package::dependents(const char *const name)
{
	return Package::filter([name](Package& pkg) {
		for (std::string& depName: pkg.dependencies) {
			if (depName == name) {
				return true;
			}
		}
		return false;
	});
}

std::string Package::linkableObject(const std::filesystem::path &p)
{
	std::string fileName = p.filename();

	if (fileName.starts_with("lib")) {
		// remove "lib" from the beginning
		fileName = fileName.substr(3);
	}

	// remove extension ".a"
	return fileName.substr(0, fileName.length() - 2);
}

bool Package::build(const Package &pkg)
{
	if (!Package::generateCmake(pkg)) {
		return false;
	}

	// build
	std::cout << "Compiling..." << std::endl;
	return utils::system::runCommand("cd " + pkg.path + " && make") == 0;
}

bool Package::check(const Package &pkg, bool strict)
{

	std::cout << "Checking " << (pkg.managed ? "managed" : "non-managed") << " package " << pkg.name << std::endl;

	auto path = Package::getPath(pkg);
	return Package::checkPath(path, pkg.managed, strict);
}

bool Package::checkPath(const std::filesystem::path& path, bool asManaged, bool strict)
{
	bool existsOnFilesystem = std::filesystem::exists(path);
	
	if (!existsOnFilesystem) {
		std::cerr << "Not found in " << path << std::endl;
		return false;
	}
	
	if (!asManaged) {
		// non managed packages only need to exist in the file system
		return true;
	}
	
	// managed package checks
	
	// check for /src
	auto srcPath = utils::fs::extendPath<1>(path, { "src" });
	if (!std::filesystem::exists(srcPath)) {
		std::cerr << "Package is missing it's /src directory" << std::endl;
		return false;
	}
	
	// check for /includes
	auto includesPath = utils::fs::extendPath<1>(path, { "includes" });
	if (!std::filesystem::exists(includesPath)) {
		std::cerr << "Package is missing it's /includes directory" << std::endl;
		return false;
	}
	
	// check for /includes/src
	auto includesSrcPath = utils::fs::extendPath<2>(path, { "includes", "src" });
	if (!std::filesystem::exists(includesSrcPath)) {
		std::cerr << "Package is missing it's /includes/src directory" << std::endl;
		return false;
	}
	
	// check for /includes/lib
	auto includesLibPath = utils::fs::extendPath<2>(path, { "includes", "lib" });
	if (!std::filesystem::exists(includesLibPath)) {
		std::cerr << "Package is missing it's /includes/lib directory" << std::endl;
		return false;
	}
	
	if (strict) {
		// strict mode checks for files that can be generated, so they don't have to exist
		// in strict mode, must contain premake5.lua
		auto luaPath = utils::fs::extendPath<1>(path, { "premake5.lua" });
		if (!std::filesystem::exists(luaPath)) {
			std::cerr << "Package is missing premake5.lua" << std::endl;
			return false;
		}
	}
	
	std::cout << "Package valid" << std::endl;
	
	return true;
}

void Package::push(const Package &pkg)
{
	// check if .git directory exists
	const auto gitDir = Package::getPath<1>(pkg, { ".git" });
	if (!std::filesystem::exists(gitDir)) {
		std::cerr << ".git directory not found within package directory, aborted." << std::endl;
		return;
	}

	const auto pkgDir = Package::getPath(pkg);

	// materialize dependencies so the package is portable
	Package::materializeDependencies(pkg);

	// get value of CPPM_ENABLE_GIT
	// user may have set it to "1", we want to restore it to what it was later
	const char* userEnableGitValue = getenv("CPPM_ENABLE_GIT") == nullptr ? "0" : getenv("CPPM_ENABLE_GIT");

	// enable 'git push'
	setenv("CPPM_ENABLE_GIT", "1", 1);

	// track includes
	Package::gitSetTrackIncludes(pkg, true, Empty());

	// stage ./includes
	utils::system::runCommand("cd " + pkgDir.string() + " && git add -f includes/");

	std::cout << "Checking dependencies for changes" << std::endl;

	// check if there are changes in staged ./includes
	bool dependenciesChanged = utils::system::runCommand("cd " + pkgDir.string() + " && git diff --staged --exit-code --quiet includes/") != 0;

	if (dependenciesChanged) {
		// dependencies changed, commit
		std::cout << "Dependencies changed and will be committed" << std::endl;
		utils::system::runCommand("cd " + pkgDir.string() + " && git commit -m \"Dependency changes\"");

		// push to remote
		utils::system::runCommand("cd " + pkgDir.string() + " && CPPM_ENABLE_GIT=1 git push origin main");
		
		// untrack includes
		Package::gitSetTrackIncludes(pkg, false, Empty());

		// unmaterialize
		Package::unmaterializeDependencies(pkg);

	} else {
		// no dependecy changes, unstage
		std::cout << "Dependencies unchanged" << std::endl;
		utils::system::runCommand("cd " + pkgDir.string() + " && git reset includes/");
	}

	// push to remote
	const std::string command = "cd " + pkgDir.string() + " && CPPM_ENABLE_GIT=1 git push origin main";
	utils::system::runCommand(command);

	// restore initial value of CPPM_ENABLE_GIT
	setenv("CPPM_ENABLE_GIT", userEnableGitValue, 1);
}

void Package::gitSetTrackIncludes(const Package& pkg, bool track, Maybe<std::filesystem::path> pathCurrent)
{
	if (!pkg.managed) {return;}

	const auto pkgDir = Package::getPath(pkg);
	std::filesystem::path path = "";
	std::filesystem::path pathRelative = "";
	if (std::holds_alternative<std::filesystem::path>(pathCurrent)) {
		// resume from given path
		path = std::get<std::filesystem::path>(pathCurrent);
	} else {
		// start from includes
		path = Package::getPath<1>(pkg, { "includes" });
	}
	
	pathRelative = std::filesystem::path(path.string().substr(pkgDir.string().length() + 1));

	const auto pIter = std::filesystem::directory_iterator(path);
	for (auto f: pIter) {
		if (f.is_directory()) {
			// resume recursively
			Package::gitSetTrackIncludes(pkg, track, f.path());
		} else {
			// file, run git update-index --assume-unchanged path
			utils::system::runCommand(
				"cd " + pkgDir.string() +
				" && git update-index --" + (track ? "no-" : "") + "assume-unchanged " + f.path().string() +
				" 2>/dev/null"
			);
		}
	}
}

bool Package::checkAll()
{
	bool foundInvalid = false;
	std::vector<Package> packages = Package::packages();
	for (const Package& pkg: packages) {
		bool valid = Package::check(pkg, true);
		if (!valid) {
			foundInvalid = true;
		}
		std::cout << std::endl;
	}

	return !foundInvalid;
}

void Package::fixPath(const std::filesystem::path &path)
{
	// define required paths
	std::vector<std::filesystem::path> pathsRequired = {
		path,
		utils::fs::extendPath<1>(path, { "src" }),
		utils::fs::extendPath<1>(path, { "includes" }),
		utils::fs::extendPath<2>(path, { "includes", "src" }),
		utils::fs::extendPath<2>(path, { "includes", "lib" })
	};

	// check each required path
	for (std::filesystem::path& pathRequired: pathsRequired) {
		if (!std::filesystem::exists(pathRequired)) {
			std::cout << "Create directory " << pathRequired << "? (Y)Yes / (N)No" << std::endl;
			char action;
			while (true) {
				std::cin >> action;
				if (action == 'Y' || action == 'y') {
					std::filesystem::create_directory(pathRequired);
					break;
				}

				if (action == 'N' || action == 'n') {
					// won't conform, but it was user's choice
					break;
				}
			}
		}
	}
}

bool Package::generateCmake(const Package &pkg)
{
	std::cout << "Generating cmake..." << std::endl;
	return utils::system::runCommand("cd " + pkg.path + " && premake5 gmake > /dev/null") == 0;
}

bool Package::isLibrary(const Package &pkg)
{
	return pkg.type == PackageType::StaticLib || pkg.type == PackageType::SharedLib;
}

void Package::generatePremake(const Package &pkg)
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
	fstream << "\tincludedirs { \"./includes/src/**\" }" << std::endl;
	fstream << "\tlibdirs { \"./includes/lib/**\" }" << std::endl;

	if (pkg.dependencies.size() > 0) {
		// link libraries

		// collect all linked objects in a vector
		std::vector<std::string> linked;

		for (const std::string& depName: pkg.dependencies) {
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
			
			if (!dependencyPkg.managed || dependencyPkg.linkableObjects.size() > 0) {
				// has linkable objects, include them
				for (std::string link: dependencyPkg.linkableObjects) {
					std::string linkName = Package::linkableObject(std::filesystem::path(link));
					linked.push_back(linkName);
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

			for (int i = 0; i < linked.size(); i++) {
				std::string& link = linked[i];
				bool last = i == linked.size() - 1;
				fstream << "\t\t\"" << link  << '"' << (last ? "" : ",") << std::endl;
			}

			fstream << "\t}\n" << std::endl;
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
		} else {
			// pkg is a library with no linkableObjects
			// assume pkg.name
			fstream << "\t\t\"" << pkg.name << "\"" << std::endl;
		}

		fstream << "\t}" << std::endl;

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

void Package::generateVSC(Package &pkg)
{
	boost::json::object root;
	boost::json::array configurations;
	boost::json::object configuration;
	boost::json::array includePaths;

	// allow includes from current project
	includePaths.push_back("${workspaceFolder}/src/**/*");
	includePaths.push_back("${workspaceFolder}/includes/src/**/*");

	configuration["name"] = pkg.name;
	configuration["includePath"] = includePaths;
	configuration["defines"] = boost::json::array({});
	configuration["cppStandard"] = "c++20";

	configurations.push_back(configuration);

	root["configurations"] = configurations;
	root["version"] = 4;

	std::filesystem::path vscDir = std::filesystem::path(pkg.path).append(".vscode");

	if (!std::filesystem::exists(vscDir)) {
		std::filesystem::create_directory(vscDir);
	}

	std::filesystem::path configPath = std::filesystem::path(vscDir).append("c_cpp_properties.json");

	utils::json::write(configPath, root);

	std::cout << "Generated VSC configuration " << configPath.string() << std::endl;
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
	std::filesystem::path path = Core::filePath("registry.json");
	utils::json::write(path, json);
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

void Package::removeFromRegistry(Package &pkg)
{
	boost::json::array packages = Package::packagesJSON();
	boost::json::array packagesNew = {};
	
	for (auto package: packages) {
		if (package.at("name").as_string() != pkg.name) {
			packagesNew.push_back(package);
		}
	}

	Package::writeRegistry(packagesNew);
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

void Package::addSrc(const Package& pkg, const std::string& srcName)
{
	// get path to /src
	const std::filesystem::path src = Package::getPath<1>(pkg, { "src" });

	// construct file names with extensions
	const std::string cppName = srcName + ".cpp";
	const std::string hName = srcName + ".h";

	// construct paths for both files
	const std::filesystem::path cppPath = std::filesystem::path(src).append(cppName);
	const std::filesystem::path hPath = std::filesystem::path(src).append(hName);

	// becomes true if at least one file gets created
	bool added = false;

	// create blank files
	if (!std::filesystem::exists(cppPath)) {
		std::ofstream f(cppPath);
		if (f.is_open()) {
			f << std::endl;
		}
		f.close();
		added = true;
	} else {
		std::cout << ".cpp file exists, skipping" << std::endl;
	}

	if (!std::filesystem::exists(hPath)) {
		std::ofstream f(hPath);
		if (f.is_open()) {
			f << "#pragma once\n" << std::endl;
		}
		f.close();
		added = true;
	} else {
		std::cout << ".h file exists, skipping" << std::endl;
	}

	// re-generate premake
	Package::generateCmake(pkg);

	if (added) {
		std::cout << "Source " << srcName << " added" << std::endl;
	} else {
		std::cout << "No files were created" << std::endl;
	}
}

void Package::addSrc(const char *const pkgName, const char *srcName)
{
	MaybePackage pkg = Package::get(pkgName);

	if (std::holds_alternative<PackageNotFound>(pkg)) {
		std::cerr << "Package " << pkgName << " does not exist" << std::endl;
		return;
	}

	Package::addSrc(std::get<Package>(pkg), srcName);
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

void Package::listDependencies(const Package &pkg)
{
	const auto dependencies = Package::getDependencies(pkg);
	if (dependencies.size() == 0) {
		std::cout << "No dependencies" << std::endl;
	} else {
		std::cout << "+ Dependencies:" << std::endl;
		for (const Package& dep: dependencies) {
			std::cout << "|- " << dep.name << std::endl;
		}
	}
}

void Package::listDependents(const Package &pkg)
{
	const std::vector<Package> dependents = Package::dependents(pkg.name.c_str());

	if (dependents.size() == 0) {
		std::cout << "No dependents" << std::endl;
	} else {
		std::cout << "+ Dependents:" << std::endl;
		for (const Package& dep: dependents) {
			std::cout << "|- " << dep.name << std::endl;
		}
	}
}

void Package::vcpkgRegister(const char* pkgName)
{

	std::string vcpkgDir = utils::system::appDataDir() + std::filesystem::path::preferred_separator + ".vcpkg";

	if (!std::filesystem::exists(vcpkgDir)) {
		std::cerr << "Is vcpkg installed? Did not find " << vcpkgDir << std::endl;
		return;
	}

	// command to list dependencies
	std::string command = "vcpkg depend-info ";
	command += pkgName;
	command += " 2>&1";

	// read output of vcpkg depend-info into a string
	FILE* pipe = popen(command.c_str(), "r");

	if (!pipe) {
		std::cerr << "Error running command vcpkg depend-info" << std::endl;
		return;
	}

	char buffer[128];
	std::string result = "";
	while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
		result += buffer;
	}
	pclose(pipe);

	// split at newline char
	std::vector<std::string> lines = utils::string::split(result, "\n");

	// split each line at ":"
	std::vector<std::vector<std::string>> packagesWithDepstring;
	
	std::transform(
		lines.begin(),
		lines.end(),
		std::back_inserter(packagesWithDepstring),
		[](std::string& line) {
			// std::cout << "L: " << line << std::endl;
			return utils::string::split(line, ":");
		}
	);
	
	
	for (auto pkg: packagesWithDepstring) {
		if (pkg.size() > 1) {
			std::string packageName = pkg[0];
			std::string depsString = pkg[1];

			auto pkgPath = Package::vcpkgPackagePath(packageName);

			if (std::holds_alternative<Empty>(pkgPath)) {
				std::cerr << "Could not find vcpkg package path for " << packageName << std::endl;
				continue;
			}

			
			Package::registerPackage(std::get<std::filesystem::path>(pkgPath), false, packageName.c_str());
			MaybePackage pkg = Package::get(packageName.c_str());

			if (std::holds_alternative<PackageNotFound>(pkg)) {
				std::cerr << "Package " << packageName << " expected to be registered at this point" << std::endl;
				continue;
			}

			// add dependencies
			const auto depNames = utils::string::split(depsString, ", ");
			
			for (const std::string& depName: depNames) {
				std::string depnameClean = utils::string::trim(depName);
				if (depnameClean.length() == 0) {
					continue;
				}
				MaybePackage depMaybe = Package::get(depnameClean.c_str());
				if (std::holds_alternative<PackageNotFound>(depMaybe)) {
					std::cerr << "Dependency of " << packageName << ", " << depnameClean << " expected to be registered at this point" << std::endl;
					continue;
				}

				Package::addDependency(std::get<Package>(pkg), depnameClean.c_str());
			}

			// std::cout << pkg[0] << " -> " << pkg[1] << std::endl;
		}
	}

}

void Package::display(const Package& pkg)
{
	std::cout << "Package name: " << pkg.name << std::endl;
	std::cout << "Version: " << pkg.version << std::endl;
	std::cout <<
		"Type: " <<
		(pkg.managed ? "managed" : "non-managed") << " " << Package::typeToString(pkg.type) <<
		std::endl;
	std::cout << "Path: " << pkg.path << std::endl;

	Package::listDependencies(pkg);
	Package::listDependents(pkg);

	if ((pkg.type == PackageType::StaticLib || pkg.type == PackageType::SharedLib) && pkg.linkableObjects.size() > 0) {
		// show linkable objects
		std::cout << "Linkable objects:" << std::endl;
		for (const std::string& objName: pkg.linkableObjects) {
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