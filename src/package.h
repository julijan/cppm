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
	static void create(const char* const name);
	static std::vector<Package> packages();
	static boost::json::array packagesJSON();
	static bool packageExists(const char* const name);
	static MaybePackage get(const char* const name);
	static MaybePackageJSON getJSON(const char* const name);
	static std::vector<Package> getDependencies(const char* const name);
	static std::string typeToString(PackageType t);
	static PackageType typeFromString(const char* const t);
	static Package fromJSON(boost::json::value data);
	static boost::json::object toJSON(Package& pkg);
private:
	static std::string promptType();
};