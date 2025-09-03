#pragma once

#include <string>
#include <variant>
#include <functional>
#include "boost/json.hpp"

#include "types.h"

class Package;

using PackageJSON = boost::json::object;
using PackageNotFound = const char*;

using MaybePackage = std::variant<Package, PackageNotFound>;
using MaybePackageJSON = std::variant<PackageJSON, PackageNotFound>;

class Package : public PackageData {
public:
	Package(
		std::string name,
		std::string path,
		PackageType type,
		bool managed
	);

	Package(
		std::string name,
		std::string path,
		PackageType type,
		std::string version,
		std::vector<std::string> linkableObjects,
		std::vector<std::string> dependencies,
		bool managed,
		int registeredAt
	);

	// create a project (managed package)
	static void create(const char* const name);

	// creates /includes, /includes/src and /includes/lib
	static void createIncludesDirectories(const Package& pkg);
	static void createIncludesDirectories(const std::filesystem::path& p);

	// return package path
	// subdirs is an array of subdirectories within the package path
	// eg. ["includes", "lib"] -> [package.path]/includes/lib
	// passing an empty array will return the package path
	template <int Depth>
	static std::filesystem::path getPath(const char* const pkgName, SmartArray<const char*, Depth> subdirs);

	// return package path, or a path within it
	// Make sure package exists before calling this! If package does not exists path will be /dev/null
	template <int Depth>
	static std::filesystem::path getPath(const Package& pkg, SmartArray<const char*, Depth> subdirs);

	// return package path
	static std::filesystem::path getPath(const Package& pkg);

	// registers given path as a non-managed package
	static void registerPackage(const std::filesystem::path& p, bool managed);

	// unregisters given package
	static void unregisterPackage(const char* const name);

	// return all registered packages
	static std::vector<Package> packages();

	// return all registered packages as boost::json:array
	static boost::json::array packagesJSON();

	// check if package with given name exists
	static bool packageExists(const char* const name);

	// filter packages using given predicate
	static std::vector<Package> filter(std::function<bool(Package&)> predicate);

	// returns the first package matching the given predicate
	static MaybePackage find(std::function<bool(Package&)> predicate);

	// if given package exists, returns std::variant holding Package instance
	static MaybePackage get(const char* const name);

	// if given package exists, returns std::variant holding PackageJSON
	static MaybePackageJSON getJSON(const char* const name);

	// write JSON to package registry
	static void writeRegistry(const boost::json::value& json);

	// initialize registry with an empty array
	static void initRegistry();

	// add given package to registry
	static void addToRegistry(Package& pkg);

	// remove given package from registry
	static void removeFromRegistry(Package& pkg);

	// update given package in registry
	static void updateRegistry(Package& pkg);

	// create /src/[name].cpp and /src/[name].h
	static void addSrc(const Package& pkg, const std::string& srcName);
	// create /src/[name].cpp and /src/[name].h
	static void addSrc(const char* const pkgName, const char* srcName);

	// return package dependency names
	static std::vector<std::string> dependencyNames(const char* const name);

	// return package dependencies
	static std::vector<Package> getDependencies(const char* const name);

	// return package dependencies
	static std::vector<Package> getDependencies(const Package& pkg);

	// add dep as dependency of pkg
	static void addDependency(Package& pkg, Package& dep);

	// add name as dependency of given Package
	static void addDependency(Package& pkg, const char* const name);

	// remove dep as dependency of pkg
	static void removeDependency(Package& pkg, Package& dep);

	// remove name as dependency of given Package
	static void removeDependency(Package& pkg, const char* const name);

	// (re)create links for all package dependencies
	static void linkDependencies(const Package&pkg);

	// check if depName is a dependency of pkg
	static bool isDependency(Package&pkg, const char* const depName);

	// check if dep is a dependency of pkg
	static bool isDependency(Package&pkg, Package& dep);

	// check if depName is a dependency of pkgName
	static bool isDependency(const char* const pkgName, const char* const depName);

	// materialize dependencies
	// copy contents of all dependencies into includes directory instead of symlinks
	// this makes the project portable but consumes more space
	// used before cppm push
	static void materializeDependencies(const Package& pkg);

	// unmaterialize dependencies
	// reverse of materialize dependencies, deletes everything from includes and creates symbolic links
	static void unmaterializeDependencies(const Package& pkg);

	// returns packages that depend on given package
	static std::vector<Package> dependents(const char* const name);

	// true if StaticLib or SharedLib
	static bool isLibrary(const Package& pkg);

	// generate premake5.lua for given Package
	static void generatePremake(const Package& pkg);

	// generate VSC (Visual Studio Code) configuration for given package
	static void generateVSC(Package& pkg);

	// (re)generate cmake
	static bool generateCmake(const Package& pkg);

	// build given package
	static bool build(const Package& pkg);

	// verify package integrity
	// non-managed: must exist on the filesystem
	// managed must exist on the filesystem and:
	// have /src
	// have premake5.lua (if strict = true)
	static bool check(const Package& pkg, bool strict);

	static bool checkAll();

	// does given path contain a package-like structure
	static bool checkPath(const std::filesystem::path& path, bool asManaged, bool strict);

	// interactively fix path - make given path conform to managed-package structure
	static void fixPath(const std::filesystem::path& path);

	// returns project in given path, or path containing given path
	// for example, project in /a will be returned by path /a or /a/b or /a/b/c but wont by /b/c
	static MaybePackage includesPath(std::filesystem::path p);

	// returns project in given path
	static MaybePackage inPath(std::filesystem::path p);

	// convert from enum PackageType to string
	static std::string typeToString(PackageType t);

	// list dependencies of given Package
	static void listDependencies(Package& pkg);

	// display package details given Package instance
	static void display(Package& pkg);

	// display package details given package name
	static void display(const char* const name);

	// convert string to enum PackageType
	static PackageType typeFromString(const char* const t);

	// given a PackageJSON returns instance of Package
	static Package fromJSON(PackageJSON data);

	// given a Package instance returns PackageJSON
	static PackageJSON toJSON(Package& pkg);
private:
	// prompt user to select a package type
	static std::string promptType(bool expectLibrary);
};