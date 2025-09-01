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

	// if given package exists, returns std::variant holding PackageJSON
	static MaybePackageJSON getJSON(const char* const name);

	// return package dependency names
	static std::vector<std::string> dependencyNames(const char* const name);

	// return package dependencies
	static std::vector<Package> getDependencies(const char* const name);

	// returns project in given path, or path containing given path
	// for example, project in /a will be returned by path /a or /a/b or /a/b/c but wont by /b/c
	static MaybePackage includesPath(std::filesystem::path p);

	// returns project in given path
	static MaybePackage inPath(std::filesystem::path p);

	// convert from enum PackageType to string
	static std::string typeToString(PackageType t);

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