#include <iostream>
#include <cstring>
#include <algorithm>
#include <filesystem>

#include "format.h"

#include "PrintNice.h"
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
			PrintNice::print(
				fmt::format("Can't create a package within a package. Current package is {}", currentPackageName),
				OutputType::Warning
			);
			return 1;
		}
		return Package::create(argv[2]) ? 0 : 1;
	}

	if (strcmp(command, "list") == 0 || strcmp(command, "ls") == 0) {
		// list packages
		Package::list();
		return 0;
	}

	if (strcmp(command, "show") == 0) {
		// show details of provided package
		// if package not provided show details of current package
		if (argc == 2) {
			// current project
			if (!isPackage) {
				PrintNice::warning(
					"show command can be used without arguments only from a package directory. Try cppm show [packageName]."
				);
				return 1;
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
				PrintNice::warning("Invalid use of include command.");
				help::printHelp("include");
				return 1;
			}

			Package pkg = std::get<Package>(package);

			if (!pkg.managed) {
				PrintNice::warning("Can't use include in a non-managed package");
				return 1;
			}

			Package::listDependencies(pkg);
			return 0;
		}

		// add listed package(s) as dependency of current package
		if (!isPackage) {
			PrintNice::warning("To include a package as a dependency, you must be within an existing package");
			return 1;
		}
		
		Package pkg = std::get<Package>(package);

		if (!pkg.managed) {
			PrintNice::warning("Can't use include in a non-managed package");
			return 1;
		}

		for (int i = 2; i < argc; i++) {
			Package::addDependency(pkg, argv[i]);
		}

		return 0;
	}

	if (strcmp(command, "exclude") == 0) {
		// remove a dependency

		if (!isPackage) {
			PrintNice::warning("To remove a dependency you must be withn a package.");
			return 1;
		}

		if (argc < 3) {
			PrintNice::warning("You must list at least one package to exclude");
			return 1;
		}

		Package pkg = std::get<Package>(package);

		if (!pkg.managed) {
			PrintNice::warning("Can't use exclude in a non-managed package");
			return 1;
		}

		for (int i = 2; i < argc; i++) {
			Package::removeDependency(pkg, argv[i]);
		}

		return 0;
	}

	if (strcmp(command, "register") == 0) {
		bool asManaged = argc > 2 ? strcmp(argv[2], "managed") == 0 : false;
		// register current path as a package
		Package::registerPackage(path, asManaged, nullptr);
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

	if (strcmp(command, "vcpkg-register") == 0) {
		// register given package and all it's dependencies from vcpkg
		if (argc == 2) {
			std::cerr << "Package name not provided" << std::endl;
			return 0;
		}

		for (int i = 2; i < argc; i++) {
			std::cout << "Registering " << argv[i] << " and it's dependencies" << std::endl;
			Package::vcpkgRegister(argv[i]);
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
				PrintNice::warning(
					"Command build has be executed within a package, or provided a list of package names to build"
				);
				return 1;
			}

			const Package& pkg = std::get<Package>(package);
			return Package::build(pkg) ? 0 : 1;
		}

		bool someFailed = false;
		for (int i = 2; i < argc; i++) {
			MaybePackage pkgMaybe = Package::get(argv[i]);
			if (std::holds_alternative<PackageNotFound>(pkgMaybe)) {
				PrintNice::warning(fmt::format("Package {} not found, skipped", argv[i]));
				continue;
			}

			const Package& pkg = std::get<Package>(pkgMaybe);
			if (!Package::build(pkg)) {
				someFailed = true;
			}
		}

		return someFailed ? 1 : 0;
	}

	if (strcmp(command, "add") == 0) {
		// for each listed file
		// create /src/[file].cpp and /src/[file].h

		if (!isPackage) {
			std::cerr << "Add command can only be executed within a package" << std::endl;
			return 0;
		}

		if (argc < 3) {
			std::cerr << "You must list at least one source file to be created" << std::endl;
			return 0;
		}

		for (int i = 2; i < argc; i++) {
			Package::addSrc(currentPackageName.c_str(), argv[i]);
		}

		return 0;
	}

	if (strcmp(command, "check") == 0) {
		// check package(s) making sure they exist and conform to registry

		if (argc == 2) {
			// no arguments: check current package
			if (!isPackage) {
				PrintNice::print(
					"To use check without arguments you must be within a package directory.",
					OutputType::Warning
				);
				return 1;
			}

			const Package& pkg = std::get<Package>(package);
			return Package::check(pkg, true) ? 0 : 1;
		}

		if (argc == 3 && strcmp(argv[2], "--all") == 0) {
			// --all: check all packages
			return Package::checkAll() ? 0 : 1;
		}

		// package list: check listed packages
		for (int i = 2; i < argc; i++) {
			MaybePackage pkgMaybe = Package::get(argv[i]);
			if (std::holds_alternative<PackageNotFound>(pkgMaybe)) {
				PrintNice::print(
					fmt::format("Package {} not registered, skipping\n", argv[i]),
					OutputType::Warning
				);
				continue;
			}
			const Package& pkg = std::get<Package>(pkgMaybe);
			Package::check(pkg, true);
			PrintNice::print();
		}
		return 0;
	}

	if (strcmp(command, "mv") == 0 || strcmp(command, "move") == 0) {
		// move project to given path updating dependendents

		if (argc == 2) {
			std::cerr << "Not enough arguments, run 'cppm help mv' for information on how to use it" << std::endl;
			return 0;
		}

		if (argc == 3) {

			if (!isPackage) {
				std::cerr << "Not within a package and package name not provided" << std::endl;
				return 0;
			}

			const char* pathString = argv[2];
			if (strlen(pathString) == 0) {
				std::cerr << "Invalid path" << std::endl;
				return 0;
			}

			// moving from within a package, not ideal for the user
			// as they will remain in a non existing path in terminal, which can cause confusion
			std::cout << "You are moving a package while being in it's directory. This is possible, but your terminal working directory will remain the current directory, which may no longer exist after this. This is usually not a problem, but it can cause confusion." << std::endl;
			std::cout << "You can proceed, or cd out of the package and run cppm mv [package] [path]." << std::endl;
			std::cout << "(P)Proceed / (A)Abort" << std::endl;
			char action;
			while (true) {
				std::cin >> action;

				if (action == 'P' || action == 'p') {
					break;
				}

				if (action == 'A' || action == 'a') {
					return 0;
				}
			}

			// user decided to proceed
			Package pkg = std::get<Package>(package);
			std::filesystem::path to(pathString);

			Package::move(pkg, to);

			return 0;
		}

		const char* pkgName = argv[2];
		const char* pathToString = argv[3];

		MaybePackage pkg = Package::get(pkgName);
		if (std::holds_alternative<PackageNotFound>(pkg)) {
			std::cerr << "Package " << pkgName << " not found" << std::endl;
			return 0;
		}

		const std::filesystem::path pathTo(pathToString);
		Package::move(std::get<Package>(pkg), pathTo);

		return 0;
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

	if (strcmp(command, "cd") == 0) {
		// cd to project directory
	}

	if (strcmp(command, "open") == 0) {
		// open project in IDE
	}

	if (strcmp(command, "materialize") == 0) {
		// materialize dependencies
		if (argc == 2) {

			if (!isPackage) {
				std::cerr << "Not within package and package name not provided" << std::endl;
				return 0;
			}

			// materialize current package
			const Package pkg = std::get<Package>(package);
			Package::materializeDependencies(pkg);

			return 0;
		}

		for (int i = 2; i < argc; i++) {
			MaybePackage pkg = Package::get(argv[i]);
			if (std::holds_alternative<PackageNotFound>(pkg)) {
				std::cerr << "Skipped package " << argv[i] << ", not found" << std::endl;
				continue;
			}
			Package::materializeDependencies(std::get<Package>(pkg));
		}
	}

	if (strcmp(command, "unmaterialize") == 0) {
		// unmaterialize dependencies
		if (argc == 2) {

			if (!isPackage) {
				std::cerr << "Not within package and package name not provided" << std::endl;
				return 0;
			}

			// materialize current package
			const Package pkg = std::get<Package>(package);
			Package::unmaterializeDependencies(pkg);

			return 0;
		}

		for (int i = 2; i < argc; i++) {
			MaybePackage pkg = Package::get(argv[i]);
			if (std::holds_alternative<PackageNotFound>(pkg)) {
				std::cerr << "Skipped package " << argv[i] << ", not found" << std::endl;
				continue;
			}
			Package::unmaterializeDependencies(std::get<Package>(pkg));
		}
	}

	if (strcmp(command, "push") == 0) {
		// push to remote
		// this will first materialize dependencies and then push
		// otherwise the project on remote will contain symlinks pointing to non-existent data
		// after push, dependencies will get de-materialized
		if (argc == 2) {
			// package not provided, assume current
			if (!isPackage) {
				std::cerr << "You must provide a package name or execute within a package" << std::endl;
				return 0;
			}

			Package::push(std::get<Package>(package));
			return 0;
		}

		for (int i = 2; i < argc; i++) {
			MaybePackage pkg = Package::get(argv[i]);
			if (std::holds_alternative<PackageNotFound>(pkg)) {
				std::cerr << "Skipped package " << argv[i] << ", not found" << std::endl;
				continue;
			}
			Package::push(std::get<Package>(pkg));
		}

		return 0;
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