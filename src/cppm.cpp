#include <iostream>
#include <cstring>

#include "package.h"
#include "core.h"


int main(int argc, const char* argv[]) {
	
	// make sure app dir and necessary files are created
	Core::initDirectory();

	if (argc == 1) {
		// called with no arguments
		// output help
		Core::printHelp(nullptr);
		return 0;
	}

	const char* const command = argv[1];

	if (strcmp(command, "init") == 0) {
		// initialize a c++ project (managed package)
		Package::create(argv[2]);
		return 0;
	}

	if (strcmp(command, "list") == 0 || strcmp(command, "ls") == 0) {
		// list packages
	}

	if (strcmp(command, "include") == 0) {
		// include file or remote package or list dependencies if no arguments provided
	}

	if (strcmp(command, "packages") == 0) {
		// no arguments: list packages available in the registry
		// check: check all packages in the registry making sure they exist and conform to registry
	}

	if (strcmp(command, "register") == 0) {
		// register current path as a package
	}

	if (strcmp(command, "unregister") == 0) {
		// unregister provided package name, if omiitted unregister the current path as a package
	}

	if (strcmp(command, "refresh") == 0) {
		// refresh given package, look for linkable objects
	}

	if (strcmp(command, "build") == 0) {
		// build the project
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
		// help [command] - show options for command
	}

	std::cerr << "Command " << command << " not recognized" << std::endl;

	return 0;
}