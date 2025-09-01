#pragma once

#include <string>
#include <variant>
#include "boost/json.hpp"
#include "types.h"

class Package;

using MaybePackage = std::variant<Package, const char*>;
using MaybePackageJSON = std::variant<boost::json::object, const char*>;

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

	// if given package exists, returns std::variant holding Package instance
	static MaybePackage get(const char* const name);

	// if given package exists, returns std::variant holding boost::json::object
	static MaybePackageJSON getJSON(const char* const name);

	// return package dependencies
	static std::vector<Package> getDependencies(const char* const name);

	// convert from enum PackageType to string
	static std::string typeToString(PackageType t);

	// convert string to enum PackageType
	static PackageType typeFromString(const char* const t);

	// given a boost::json::object returns instance of Package
	static Package fromJSON(boost::json::object data);

	// given a Package instance returns boost::json::object
	static boost::json::object toJSON(Package& pkg);
private:
	// prompt user to select a package type
	static std::string promptType();
};