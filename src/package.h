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

	// write JSON to package registry
	static void writeRegistry(const boost::json::value& json);

	// initialize registry with an empty array
	static void initRegistry();

	// update given package in registry
	static void updateRegistry(Package& pkg);

	// if given package exists, returns std::variant holding PackageJSON
	static MaybePackageJSON getJSON(const char* const name);

	// return package dependency names
	static std::vector<std::string> dependencyNames(const char* const name);

	// return package dependencies
	static std::vector<Package> getDependencies(const char* const name);

	// add dep as dependency of pkg
	static void addDependency(Package& pkg, Package& dep);

	// add name as dependency of given Package
	static void addDependency(Package& pkg, const char* const name);

	// remove dep as dependency of pkg
	static void removeDependency(Package& pkg, Package& dep);

	// remove name as dependency of given Package
	static void removeDependency(Package& pkg, const char* const name);

	// check if depName is a dependency of pkg
	static bool isDependency(Package&pkg, const char* const depName);

	// check if dep is a dependency of pkg
	static bool isDependency(Package&pkg, Package& dep);

	// check if depName is a dependency of pkgName
	static bool isDependency(const char* const pkgName, const char* const depName);

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
	static std::string promptType();
};