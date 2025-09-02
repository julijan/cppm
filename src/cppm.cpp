#include <iostream>
#include <cstring>
#include <algorithm>
#include <filesystem>

#include "package.h"
#include "core.h"
#include "help.h"
#include "utils.h"


int main(int argc, const char* argv[]) {

	// make sure app dir and necessary files are created
	Core::initDirectory();

	const std::filesystem::path path = utils::fs::currentPath();

	// get active package
	const MaybePackage package = Package::includesPath(path);

	// true if current path is within one of the packages
	const bool isPackage = std::holds_alternative<Package>(package);

	const std::string currentPackageName = isPackage ? std::get<Package>(package).name : "none";

	if (argc == 1) {
		// called with no arguments
		// output help
		help::printHelp();
		return 0;
	}

	const char* const command = argv[1];

	if (strcmp(command, "init") == 0) {
		// initialize a c++ project (managed package)
		if (isPackage) {
			// disallow creating a package within existing package
			std::cerr << "Can't create a package within a package. Current package is " << currentPackageName << std::endl;
			return 0;
		}
		Package::create(argv[2]);
		return 0;
	}

	if (strcmp(command, "list") == 0 || strcmp(command, "ls") == 0) {
		// list packages
		std::vector<Package> packages = Package::packages();
		std::sort(packages.begin(), packages.end(), [](auto a, auto b) {
			int aVal = a.managed ? 1 : 0;
			int bVal = b.managed ? 1 : 0;
			return aVal > bVal;
		});
		for (auto package: packages) {
			std::cout << package.name << std::endl;
		}
		return 0;
	}

	if (strcmp(command, "show") == 0) {
		// show details of provided package
		// if package not provided show details of current package
		if (argc == 2) {
			// current project
			if (!isPackage) {
				std::cerr << "show command can be used without arguments only from a package directory. Try cpmm show [packageName]." << std::endl;
				return 0;
			}
			Package pkg = std::get<Package>(package);
			Package::display(pkg);
			return 0;
		}

		if (argc > 2) {
			Package::display(argv[2]);
			return 0;
		}
	}

	if (strcmp(command, "include") == 0) {
		// include package or remote package or list dependencies if no arguments provided
		if (argc == 2) {
			// no arguments provided
			// if within a package, list dependencies

			if (!isPackage) {
				std::cout << "Invalid use of include command. Try cppm help include." << std::endl;
				return 0;
			}

			Package pkg = std::get<Package>(package);

			if (!pkg.managed) {
				std::cerr << "Can't use include in a non-managed package" << std::endl;
				return 0;
			}

			Package::listDependencies(pkg);
			return 0;
		}

		// add listed package(s) as dependency of current package
		if (!isPackage) {
			std::cerr << "To include a package as a dependency, you must be within an existing package" << std::endl;
			return 0;
		}
		
		Package pkg = std::get<Package>(package);

		if (!pkg.managed) {
			std::cerr << "Can't use include in a non-managed package" << std::endl;
			return 0;
		}

		for (int i = 2; i < argc; i++) {
			Package::addDependency(pkg, argv[i]);
			std::cout << argv[2] << " added as a dependecy of " << pkg.name << std::endl;
		}

		return 0;
	}

	if (strcmp(command, "exclude") == 0) {
		// remove a dependency

		if (!isPackage) {
			std::cerr << "To remove a dependency you must be withn a package." << std::endl;
			return 0;
		}

		if (argc < 3) {
			std::cerr << "You must list at least one package to exclude" << std::endl;
			return 0;
		}

		Package pkg = std::get<Package>(package);

		if (!pkg.managed) {
			std::cerr << "Can't use exclude in a non-managed package" << std::endl;
			return 0;
		}

		for (int i = 2; i < argc; i++) {
			Package::removeDependency(pkg, argv[i]);
		}

		return 0;
	}

	if (strcmp(command, "register") == 0) {
		// register current path as a non-managed package
		Package::registerPackage(path);
		return 0;
	}

	if (strcmp(command, "unregister") == 0) {
		// unregister provided package name, if omiitted unregister the current path as a package
		if (argc == 2) {
			// no arguments provided, must be within package directory
			if (!isPackage) {
				std::cerr << "Using unregister without package name(s) can only be done from a project directory" << std::endl;
				return 0;
			}

			Package::unregisterPackage(currentPackageName.c_str());

			return 0;
		}

		// unregister listed packages
		for (int i = 2; i < argc; i++) {
			Package::unregisterPackage(argv[i]);
		}

		return 0;
	}

	if (strcmp(command, "vsc") == 0) {
		// generate .vscode/c_cpp_properties.json
		if (argc == 2) {
			// no arguments provided, initialize in current project
			if (!isPackage) {
				std::cerr << "Must be executed in a package directory or provided package name(s)" << std::endl;
				return 0;
			}

			// initialize in current package
			Package pkg = std::get<Package>(package);
			Package::generateVSC(pkg);
			return 0;
		}

		// initialize for all listed packages
		for (int i = 2; i < argc; i++) {
			MaybePackage pkgMaybe = Package::get(argv[i]);
			if (std::holds_alternative<PackageNotFound>(pkgMaybe)) {
				std::cerr << "Package " << argv[i] << " not found, skipping";
				continue;
			}

			Package& pkg = std::get<Package>(pkgMaybe);
			if (!pkg.managed) {
				std::cerr << "Package " << pkg.name << " is not a managed package, skipping";
				continue;
			}

			Package::generateVSC(pkg);
		}

		return 0;
	}

	if (strcmp(command, "build") == 0) {
		// build package(s)
		if (argc == 2) {
			// no arguments provided, build current package
			if (!isPackage) {
				std::cerr << "Command build has be executed within a package, or provided a list of package names to build" << std::endl;
				return 0;
			}

			const Package& pkg = std::get<Package>(package);
			Package::build(pkg);
		}

		for (int i = 2; i < argc; i++) {
			MaybePackage pkgMaybe = Package::get(argv[i]);
			if (std::holds_alternative<PackageNotFound>(pkgMaybe)) {
				std::cerr << "Package " << argv[i] << " not found, skipped" << std::endl;
				continue;
			}

			const Package& pkg = std::get<Package>(pkgMaybe);
			Package::build(pkg);
		}
	}

	if (strcmp(command, "verify") == 0) {
		// verify package(s) making sure they exist and conform to registry
		// no arguments: verify current package
		// --all: verify all packages
		// package list: verify listed packages
	}

	if (strcmp(command, "refresh") == 0) {
		// refresh given package, look for linkable objects
	}

	if (strcmp(command, "version") == 0) {
		// create a new version
		// store previous version /src in projectDir/.versions
	}

	if (strcmp(command, "run") == 0) {
		// run the project
	}

	if (strcmp(command, "mv") == 0) {
		// move project to given path updating dependendents
	}

	if (strcmp(command, "cd") == 0) {
		// cd to project directory
	}

	if (strcmp(command, "open") == 0) {
		// open project in IDE
	}

	if (strcmp(command, "help") == 0) {
		// no args: list commands with description
		if (argc == 2) {
			help::printHelp();
			return 0;
		}
		
		// help [command] - show options for command
		help::printHelp(argv[2]);

		return 0;
	}

	std::cerr << "Command " << command << " not recognized" << std::endl;

	help::printHelp();

	return 0;
}