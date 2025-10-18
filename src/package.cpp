#include <ios>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <unordered_set>
#include <variant>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>

#include "fmt/format.h"

#include "package.h"
#include "types.h"
#include "utils.h"
#include "core.h"
#include "PrintNice.h"
#include "j-utils-json/j-utils-json.h"
#include "j-utils-string/j-utils-string.h"
#include "j-utils-system/j-utils-system.h"

Package::Package(std::string name, std::string path, PackageType type, bool managed)
{
	this->name = name;
	this->path = path;
	this->type = type;
	this->managed = managed;
	this->registeredAt = utils::time::unixTimestamp();
	this->includeDirs = std::vector<std::string>();
	this->libDirs = std::vector<std::string>();
	this->linkableObjects = std::vector<std::string>();
	this->tests = std::vector<std::string>();
}

Package::Package(
	std::string name,
	std::string path,
	PackageType type,
	std::string version,
	std::vector<std::string> includeDirs,
	std::vector<std::string> libDirs,
	std::vector<std::string> linkableObjects,
	std::vector<std::string> dependencies,
	std::vector<std::string> uses,
	std::vector<std::string> tests,
	bool managed,
	int registeredAt)
{
	this->name = name;
	this->path = path;
	this->type = type;
	this->version = version;
	this->includeDirs = includeDirs;
	this->libDirs = libDirs;
	this->linkableObjects = linkableObjects;
	this->dependencies = dependencies;
	this->uses = uses;
	this->tests = tests;
	this->managed = managed;
	this->registeredAt = registeredAt;
};

bool Package::create(const char *const name)
{

	if (Package::packageExists(name)) {
		PrintNice::print(
			fmt::format("Package with name {} already exists", name),
			OutputType::Warning
		);
		return false;
	}

	// get current path
	const std::filesystem::path cwd = std::filesystem::current_path();

	// path to be created
	const std::filesystem::path projectDir = std::filesystem::path(cwd).append(name);

	// check if exists
	if (exists(projectDir)) {
		PrintNice::print(
			fmt::format("Path {} exists", projectDir.string()),
			OutputType::Warning
		);
		return false;
	}

	// project dir does not exist, create it
	try {
		std::filesystem::create_directory(projectDir);
	} catch(std::filesystem::filesystem_error e) {
		PrintNice::error(fmt::format("Error creating project directory: {}", e.what()), ErrorSeverity::Medium);
		return false;
	}

	// create sub-directories
	std::filesystem::create_directory(utils::fs::extendPath<1>(projectDir, { "src" }));
	std::filesystem::create_directory(utils::fs::extendPath<1>(projectDir, { "tests" }));
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
		PrintNice::print("Initialized git repository", OutputType::Info);
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

	// create .gitignore
	std::filesystem::path gitignorePath = Package::getPath<1>(pkg, { ".gitignore" });
	std::ofstream fsGitIgnore(gitignorePath);
	fsGitIgnore << "includes" << std::endl;
	fsGitIgnore << "bin" << std::endl;
	fsGitIgnore << "obj" << std::endl;
	fsGitIgnore << ".vscode" << std::endl;
	fsGitIgnore << "Makefile" << std::endl;
	fsGitIgnore << "*.make" << std::endl;
	fsGitIgnore << "tests/bin" << std::endl;
	fsGitIgnore << "tests/lib" << std::endl;
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
|--src (dependency include files)
|--lib (dependency lib files)
+-tests (test files will be created here)
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

	// create hello world entry point
	Package::addSrc(pkg, pkg.name);
	std::string helloWorldFileName = pkg.name + ".cpp";
	std::filesystem::path cppPath = Package::getPath<2>(pkg, { "src", helloWorldFileName.c_str() });
	std::ofstream fs(cppPath, std::ios::app);

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

	PrintNice::print(fmt::format("Package created in directory {}", name), OutputType::Success);

	return true;
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
	std::filesystem::path includesUses = utils::fs::extendPath<2>(p, { "includes", "uses" });
	std::filesystem::path includesUsesSrc = utils::fs::extendPath<3>(p, { "includes", "uses", "src" });
	std::filesystem::path includesUsesLib = utils::fs::extendPath<3>(p, { "includes", "uses", "lib" });

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
	if (!std::filesystem::exists(includesUses)) {
		std::filesystem::create_directory(includesUses);
	}
	if (!std::filesystem::exists(includesUsesSrc)) {
		std::filesystem::create_directory(includesUsesSrc);
	}
	if (!std::filesystem::exists(includesUsesLib)) {
		std::filesystem::create_directory(includesUsesLib);
	}
}

bool Package::testCreate(Package &pkg, const char *name)
{
	auto existing = std::find_if(
		pkg.tests.begin(),
		pkg.tests.end(),
		[name](std::string& testName) {
			return strcmp(testName.c_str(), name) == 0;
		}
	);

	if (existing != pkg.tests.end()) {
		PrintNice::warning(fmt::format("Test {} already exists", name));
		return false;
	}

	// make sure ./tests directory exists, create it if needed
	std::filesystem::path testsPath = Package::getPath<1>(pkg, { "tests" });
	if (!std::filesystem::exists(testsPath)) {
		std::filesystem::create_directory(testsPath);
	}

	// create cpp file with boilerplate
	std::string fileName = name;
	fileName += ".cpp";
	std::ofstream fs(Package::getPath<2>(pkg, { "tests", fileName.c_str() }));
	if (!fs.is_open()) {
		PrintNice::error("Error creating test file");
		return false;
	}
	fs << "// Test: " << name << std::endl;
	fs << "// exit code = 0 - test passed" << std::endl;
	fs << "// exit code > 0 - test failed\n" << std::endl;
	fs << "int main() {" << std::endl;
	fs << "\treturn 0;" << std::endl;
	fs << '}' << std::endl;
	fs.close();

	// update in registry
	pkg.tests.push_back(std::string(name));
	Package::updateRegistry(pkg);

	PrintNice::success(fmt::format("Test {} created", name));

	Package::generatePremake(pkg);

	return true;
}

bool Package::testRun(const Package& pkg, const char* const name)
{
	// make sure Package has given test
	auto exists = std::find_if(
		pkg.tests.begin(),
		pkg.tests.end(),
		[name](const std::string& testName) {
			return strcmp(testName.c_str(), name) == 0;
		}
	);

	if (exists == pkg.tests.end()) {
		PrintNice::warning(fmt::format("Package {} has no test {}", pkg.name, name));
		return false;
	}

	PrintNice::info(fmt::format("Running test {}", name));

	// make sure tests directory exists
	const std::filesystem::path testsPath = Package::getPath<1>(pkg, { "tests" });
	if (!std::filesystem::exists(testsPath)) {
		PrintNice::error("./tests directory missing!", ErrorSeverity::High);
		return false;
	}

	// make sure test binary exists, if not build it
	std::string binaryName = "Test-";
	binaryName += name;
	const std::filesystem::path binariesPath = utils::fs::extendPath<1>(testsPath, { "bin" });
	const std::filesystem::path testBin = utils::fs::extendPath<1>(binariesPath, { binaryName.c_str() });
	if (!std::filesystem::exists(testBin)) {
		// build test binary
		if (!Package::build(pkg, BuildConfig::Debug, binaryName.c_str())) {
			PrintNice::error(fmt::format("Compiling test {} failed", name), ErrorSeverity::Low);
			return false;
		}

		// make sure test binary exists after build
		if (!std::filesystem::exists(testBin)) {
			PrintNice::error(
				fmt::format("Test binary for test {} not found in {}", name, testBin.string()),
				ErrorSeverity::Low
			);
			return false;
		}
	}

	// test binary exists, run it
	int exitCode = utils::system::runCommand("cd " + binariesPath.string() + " && ./" + binaryName);
	bool passed = exitCode == 0;

	if (passed) {
		PrintNice::success(fmt::format("✓ Test {} passed", name));
	} else {
		PrintNice::error(fmt::format("✗ Test {} failed with exit code {}", name, exitCode));
	}

	return passed;
}

bool Package::testsRun(const Package &pkg)
{
	return Package::testsRun(pkg, pkg.tests);
}

bool Package::testsRun(const Package &pkg, const std::vector<std::string>& tests)
{
	if (tests.size() == 0) {
		// no tests specified
		PrintNice::print(fmt::format("No tests specified", pkg.name), OutputType::Normal);
		return true;
	}

	if (pkg.tests.size() == 0) {
		// no tests
		PrintNice::print(fmt::format("Package {} has no tests", pkg.name), OutputType::Normal);
		return true;
	}

	std::string message = tests.size() == pkg.tests.size() ?
		fmt::format("Running all tests for {}...\n", pkg.name) :
		fmt::format("Running {} test(s) for {}...\n", tests.size(), pkg.name);
	PrintNice::print(message.c_str(), OutputType::Info, TextStyle::Italic);

	int failed = 0;
	for (const std::string& testName: tests) {
		if (!Package::testRun(pkg, testName.c_str())) {
			failed++;
		}
		PrintNice::print();
	}

	if (failed > 0) {
		PrintNice::warning(fmt::format("{}/{} test(s) failed", failed, tests.size()));
		return false;
	}

	// all tests pass
	PrintNice::success("All tests pass!");

	return true;
}

bool Package::testRemove(Package &pkg, const char *name)
{
	const auto position = std::find_if(
		pkg.tests.begin(),
		pkg.tests.end(),
		[name](const std::string& testName) {
			return strcmp(testName.c_str(), name) == 0;
		}
	);

	if (position == pkg.tests.end()) {
		// package does not have the test
		PrintNice::warning(
			fmt::format("Can't remove test {} from package {}, test does not exist", name, pkg.name)
		);
		return false;
	}

	// confirm action
	PrintStream stream = PrintNice::stream();

	stream <<
		"You are about to remove test" <<
		TextStyledToken{
			name,
			OutputType::Info,
			TextStyle::Bold
		} <<
		"from package" << pkg.name << StreamOut();

	stream <<
		"Test source and binary will be" <<
		TextStyledToken{
			"permanently deleted",
			OutputType::Error,
			TextStyle::Bold | TextStyle::Underline
		} << StreamOut();

	stream << "Do you want to proceed?" << StreamOut();

	stream << "(Y)Yes, delete test / (N) No, cancel" << StreamOut();

	char action;
	while (true) {
		std::cin >> action;

		if (action == 'Y' || action == 'y') {
			// proceed with delete
			break;
		}

		if (action == 'N' || action == 'n') {
			// canceled
			return false;
		}
	}

	// remove test from package registry data
	pkg.tests.erase(position);
	Package::updateRegistry(pkg);

	// remove source and binary files
	std::string testSrcName = name;
	testSrcName += ".cpp";

	std::string testBinName = "Test-";
	testBinName += name;

	std::filesystem::path srcPath = Package::getPath<2>(pkg, { "tests", testSrcName.c_str() });
	std::filesystem::path binPath = Package::getPath<3>(pkg, { "tests", "bin", testBinName.c_str() });

	if (std::filesystem::exists(srcPath)) {
		std::filesystem::remove(srcPath);
	}

	if (std::filesystem::exists(binPath)) {
		std::filesystem::remove(binPath);
	}

	// re-generate premake
	Package::generatePremake(pkg);

	// done
	PrintNice::success(fmt::format("Removed test {} from {}", name, pkg.name));

	return true;
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
		PrintNice::warning(
			fmt::format("{} already registered at path {}", existingPkg.name, existingPkg.path)
		);
		return;
	}

	// if registering as managed package, make sure the package is valid
	if (managed && !Package::checkPath(p, true, false)) {
		// attempted to register directory as a managed package
		// directory is not package-like
		// offer to interactively fix package
		PrintNice::print("Current directory does not conform to managed package structure.");
		PrintNice::print("Do you want to interactively fix it so you can proceed with the operation?");
		PrintNice::print("(Y)Yes / (N)No");
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
			PrintNice::error(
				"Directory still does not conform, if you approved all fixes and you see this, report an issue",
				ErrorSeverity::Medium
			);
			return;
		}

		// if we are here, user managed to make the directory conform to managed package structure
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

	PrintStream stream = PrintNice::stream();

	stream <<
		"Registering current path as a" <<
		(managed ? "managed" : "non-managed") <<
		"package with name" <<
		TextStyledToken{
			assumedName.c_str(),
			OutputType::Info,
			0
		} << StreamOut();

	stream <<
		TextStyledToken{
			"If you want to use a different name please enter it bellow and press enter, leave blank to use",
			OutputType::Normal,
			TextStyle::Italic
		} <<
		TextStyledToken{
			assumedName.c_str(),
			OutputType::Info,
			TextStyle::Italic
		} << StreamOut();

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

	PrintNice::success(fmt::format("Package {} registered", pkg.name));
}

void Package::unregisterPackage(const char *const name)
{

	MaybePackage pkg = Package::get(name);

	if (std::holds_alternative<PackageNotFound>(pkg)) {
		PrintNice::warning(fmt::format("Package {} not registered", name));
		return;
	}

	if (std::get<Package>(pkg).managed) {
		// confirm unregistering of managed packages
		PrintStream stream = PrintNice::stream();
		stream <<
			"You are about to unregister a managed package" <<
			TextStyledToken{
				name,
				OutputType::Info,
				0
			} << StreamOut();
		
		stream << "Proceed? (Y)Yes / (N)No" << StreamOut();
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
		PrintNice::warning(fmt::format("{} packages depend on {}", dependents.size(), name));
		PrintNice::print("If you unregister this package, it will be removed from dependency list of it's dependents, as a result, affected packages may not work as expected");
		PrintNice::print("What do you want to do? (L)List dependents / (C)Cancel / (U)Unregister:");

		char action;
		while (true) {
			std::cin >> action;

			if (action == 'L' || action == 'l') {
				Package::listDependents(std::get<Package>(pkg));
			}

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

	PrintNice::success(fmt::format("Package {} unregistered", name));
}

void Package::composePackage(const char *name, std::vector<std::string> &includeDirs, std::vector<std::string> &libDirs, std::vector<std::string> &links)
{
	Package pkg(
		name,
		"scattered",
		PackageType::Composed,
		"0.1.0",
		includeDirs,
		libDirs,
		links,
		std::vector<std::string>(),
		std::vector<std::string>(),
		std::vector<std::string>(),
		false,
		utils::time::unixTimestamp()
	);
	Package::addToRegistry(pkg);

	PrintStream stream = PrintNice::stream();
	stream <<
	TextStyledToken{
		"Composed package",
		OutputType::Success,
		0
	} <<
	TextStyledToken{
		name,
		OutputType::Success,
		0
	}
	<< StreamOut();
}

std::vector<std::string> Package::findLib(const char *kw)
{
	std::string command = "pkg-config --list-all | grep ";
	command += kw;

	PrintStream stream = PrintNice::stream();
	stream <<
	TextStyledToken{
		"Running:",
		OutputType::Info,
		0
	} <<
	TextStyledToken{
		command.c_str(),
		OutputType::Normal,
		TextStyle::Italic
	} << "\n" << StreamOut();

	std::string result;
	try {
		result = utils::system::runCommandOutput(command);
	} catch(std::runtime_error e) {
		PrintNice::error(e.what());
		return std::vector<std::string>();
	}

	std::vector<std::string> lines = utils::string::split(result, "\n");

	if (lines.size() < 2) {
		PrintNice::print("No system libraries found that match given keyword", OutputType::Normal, TextStyle::Italic);
	} else {
		stream <<
		TextStyledToken{
			"Found",
			OutputType::Info,
			0
		} <<
		TextStyledToken{
			std::to_string(lines.size() - 1).c_str(),
			OutputType::Info,
			TextStyle::Bold
		} <<
		TextStyledToken{
			"libraries",
			OutputType::Info,
			0
		}
		<< StreamOut();
	}

	
	for (std::string& line: lines) {
		PrintNice::print(line);
	}

	return lines;
}

void Package::useLib(Package &pkg, std::string libName)
{

	if (!pkg.managed) {
		PrintNice::warning("use command can only be executed within a managed package");
		return;
	}

	if (utils::system::runCommand("pkg-config --exists " + libName) != 0) {
		PrintNice::warning("Library " + libName + " not found");
		return;
	}

	// library exists, check if already used by pkg
	for (std::string used: pkg.uses) {
		if (used == libName) {
			PrintNice::warning("Package " + pkg.name + " already uses " + libName);
			return;
		}
	}

	// package can be used
	pkg.uses.push_back(libName);
	Package::updateRegistry(pkg);

	// re-generate premake
	Package::generatePremake(pkg);
}

void Package::unuseLib(Package &pkg, std::string libName)
{
	if (!pkg.managed) {
		PrintNice::warning("unuse command can only be executed within a managed package");
		return;
	}

	// package can be used
	auto index = std::find_if(pkg.uses.begin(), pkg.uses.end(), [&libName](std::string& use) {
		return use == libName;
	});
	
	if (index == pkg.uses.end()) {
		PrintNice::warning("Can't unuse, " + libName + " not used by " + pkg.name);
		return;
	}

	pkg.uses.erase(index);
	Package::updateRegistry(pkg);

	// re-generate premake
	Package::generatePremake(pkg);
}

std::unordered_set<std::string> Package::useIncludeDirs(const Package &pkg)
{
	std::unordered_set<std::string> dirs;

	for (const std::string& libName: pkg.uses) {
		std::unordered_set<std::string> libIncludeDirs = Package::useIncludeDirs(pkg, libName.c_str());
		dirs.insert(libIncludeDirs.begin(), libIncludeDirs.end());
	}

	return dirs;
}

std::unordered_set<std::string> Package::useIncludeDirs(const Package &pkg, const char *libName)
{
	std::unordered_set<std::string> dirs;
	std::string command = "pkg-config --cflags-only-I ";
	command += libName;

	std::string result = "";
	try {
		result = utils::system::runCommandOutput(command);
	} catch(std::runtime_error e) {
		PrintNice::error("Failed to execute pkg-config for " + std::string(libName) + ": " + e.what());
	}

	std::vector<std::string> items = utils::string::split(result, " ");

	for (std::string& item: items) {
		// item is a path prepended by "-I"
		if (item.size() < 3) {
			continue;
		}
		
		dirs.insert(utils::string::trim(item).substr(2));
	}

	return dirs;
}

std::unordered_set<std::string> Package::useLibDirs(const Package &pkg)
{
	std::unordered_set<std::string> dirs;

	for (const std::string& libName: pkg.uses) {
		std::unordered_set<std::string> libLibDirs = Package::useLibDirs(pkg, libName.c_str());
		dirs.insert(libLibDirs.begin(), libLibDirs.end());
	}

	return dirs;
}

std::unordered_set<std::string> Package::useLibDirs(const Package &pkg, const char *libName)
{
	std::unordered_set<std::string> dirs;
	std::string command = "pkg-config --libs-only-L ";
	command += libName;

	std::string result = "";
	try {
		result = utils::system::runCommandOutput(command);
	} catch(std::runtime_error e) {
		PrintNice::error("Failed to execute pkg-config for " + std::string(libName) + ": " + e.what());
	}

	std::vector<std::string> items = utils::string::split(result, " ");

	for (std::string& item: items) {
		// item is a path prepended by "-L"
		if (item.size() < 3) {
			continue;
		}
		
		dirs.insert(utils::string::trim(item).substr(2));
	}

	return dirs;
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
	std::vector<std::string> options = expectLibrary ?
		std::vector<std::string>({ "StaticLib", "SharedLib" }) :
		std::vector<std::string>({ "ConsoleApp", "WindowedApp", "StaticLib", "SharedLib" });

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

TextStyledToken Package::managedIndicator(bool managed)
{
	return managed ?
		TextStyledToken{
			"●",
			OutputType::Success,
			0
		} :
		TextStyledToken{
			"○",
			OutputType::Normal,
			0
		};
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
		if (strcmp(package.as_object().at("name").as_string().c_str(), name) == 0) {
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

void Package::list()
{
	std::vector<Package> packages = Package::packages();
	std::sort(packages.begin(), packages.end(), [](auto a, auto b) {
		int aVal = a.managed ? 1 : 0;
		int bVal = b.managed ? 1 : 0;
		return aVal > bVal;
	});
	PrintStream stream = PrintNice::stream();
	for (auto package: packages) {
		stream << Package::managedIndicator(package.managed) << package.name.c_str() << StreamOut();
	}
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

std::unordered_set<std::string> Package::getDependenciesDeep(const Package& pkg) {
	std::unordered_set<std::string> depsDeep;

	std::vector<Package> deps = Package::getDependencies(pkg);
	for (const Package& dep: deps) {
		// insert direct dependency
		depsDeep.insert(dep.name);
		
		// include it's transient dependencies recursively
		std::unordered_set<std::string> depsTransient = Package::getDependenciesDeep(dep);
		depsDeep.insert(depsTransient.begin(), depsTransient.end());
	}

	return depsDeep;
}

void Package::addDependency(Package &pkg, Package &dep)
{
	if (Package::isDependency(pkg, dep)) {
		PrintNice::warning(fmt::format("{} already a dependency of {}", dep.name, pkg.name));
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

	PrintNice::success(fmt::format("{} added as a dependency of {}", dep.name, pkg.name));
}

void Package::addDependency(Package &pkg, const char *const name)
{
	MaybePackage dependency = Package::get(name);
	if (std::holds_alternative<PackageNotFound>(dependency)) {
		PrintNice::warning(fmt::format("Package {} does not exist", name));
		return;
	}

	Package::addDependency(pkg, std::get<Package>(dependency));
}

void Package::removeDependency(Package &pkg, Package &dep)
{
	if (!Package::isDependency(pkg, dep)) {
		PrintNice::warning(fmt::format("{} is not a dependency of {}", dep.name, pkg.name));
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

	PrintNice::success(fmt::format("Dependency {} removed", dep.name));
}

void Package::removeDependency(Package &pkg, const char *const name)
{
	MaybePackage dependency = Package::get(name);

	if (std::holds_alternative<PackageNotFound>(dependency)) {
		PrintNice::warning(fmt::format("Can't remove non-existent dependency {}", name));
		return;
	}

	Package::removeDependency(pkg, std::get<Package>(dependency));
}

void Package::linkDependency(const Package &pkg, const Package &dep)
{
	// remove existing symlinks if they exist
	Package::unlinkDependency(pkg, dep);

	if (dep.type == PackageType::Composed) {
		// dependency is a composed package, make sure there is:
		// packageDir/includes/src/dep.name and
		// packageDir/includes/lib/dep.name
		std::filesystem::path composedIncludes = Package::getPath<3>(
			pkg, { "includes", "src", dep.name.c_str() }
		);
		std::filesystem::path composedLibs = Package::getPath<3>(
			pkg, { "includes", "lib", dep.name.c_str() }
		);

		if (!std::filesystem::exists(composedIncludes)) {
			std::filesystem::create_directory(composedIncludes);
		}

		if (!std::filesystem::exists(composedLibs)) {
			std::filesystem::create_directory(composedLibs);
		}
	}

	std::vector<DependencyTarget> targets = Package::dependencyTargets(pkg, dep);

	for (DependencyTarget& target: targets) {
		std::filesystem::create_directory_symlink(target.from, target.to);
	}

	// link transient dependencies recursively
	if (dep.dependencies.size() > 0) {
		for (const std::string& depName: dep.dependencies) {
			MaybePackage dep = Package::get(depName.c_str());
			if (std::holds_alternative<PackageNotFound>(dep)) {
				PrintNice::error(
					fmt::format("Skipped linking a missing dependency {}", depName),
					ErrorSeverity::Low
				);
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
	std::vector<DependencyTarget> targets = Package::dependencyTargets(pkg, dep);

	for (DependencyTarget& target: targets) {
		if (std::filesystem::exists(target.to)) {
			std::filesystem::remove(target.to);
		}
	}

	if (dep.type == PackageType::Composed) {
		// remove no longer needed composed include dirs
		std::filesystem::path composedIncludes = Package::getPath<3>(
			pkg, { "includes", "src", dep.name.c_str() }
		);
		std::filesystem::path composedLibs = Package::getPath<3>(
			pkg, { "includes", "lib", dep.name.c_str() }
		);

		if (std::filesystem::exists(composedIncludes)) {
			std::filesystem::remove(composedIncludes);
		}

		if (std::filesystem::exists(composedLibs)) {
			std::filesystem::remove(composedLibs);
		}
	}

	// unlink transient dependencies recursively
	if (dep.dependencies.size() > 0) {
		for (const std::string& depName: dep.dependencies) {
			MaybePackage dep = Package::get(depName.c_str());
			if (std::holds_alternative<PackageNotFound>(dep)) {
				PrintNice::warning(fmt::format("Skipped unlinking a missing dependency {}", depName));
				continue;
			}

			if (!Package::isTransientDependency(pkg, std::get<Package>(dep))) {
				Package::unlinkDependency(pkg, std::get<Package>(dep));
			}
		}
	}
}

void Package::unlinkDependency(const Package &pkg, const char *depName)
{
	MaybePackage depMaybe = Package::get(depName);

	if (std::holds_alternative<PackageNotFound>(depMaybe)) {
		PrintNice::warning(fmt::format("Attempted to unlink a non-existent dependency {}", depName));
		return;
	}

	Package::unlinkDependency(pkg, std::get<Package>(depMaybe));
}

std::vector<std::filesystem::path> Package::dependencyTargetsIncludes(const Package &dep)
{
	std::filesystem::path pkgPath(Package::getPath(dep));

	std::vector<std::filesystem::path> paths;

	if (dep.type == PackageType::Composed) {
		// dependency is a composed package, include dep.includeDirs
		for (const std::string& includePathString: dep.includeDirs) {
			paths.push_back(includePathString);
		}
		return paths;
	}

	if (dep.managed) {
		// use package's src dir
		paths.push_back(utils::fs::extendPath<1>(pkgPath, { "src" }));
		return paths;
	}

	// we don't know anything about non managed packages, so we always link entire package dir
	paths.push_back(pkgPath);
	return paths;
}

std::vector<std::filesystem::path> Package::dependencyTargetsLib(const Package &dep)
{
	std::vector<std::filesystem::path> paths;
	std::filesystem::path pkgPath(Package::getPath(dep));

	if (dep.type == PackageType::Composed) {
		// dependency is a composed package, include dep.libDirs
		for (const std::string& includePathString: dep.libDirs) {
			paths.push_back(includePathString);
		}
		return paths;
	}

	if (dep.managed) {
		// use package's bin dir
		paths.push_back(utils::fs::extendPath<1>(pkgPath, { "bin" }));
		return paths;
	}

	// we don't know anything about non managed packages, so we always link entire package dir
	paths.push_back(pkgPath);
	return paths;
}

std::vector<DependencyTarget> Package::dependencyTargets(const Package& pkg, const Package& dep)
{
	std::vector<DependencyTarget> targets;

	if (pkg.managed == false) {
		// we don't manage dependencies of non-managed packages
		return targets;
	}

	std::filesystem::path pkgPath = Package::getPath(pkg);

	std::vector<std::filesystem::path> targetsInclude = Package::dependencyTargetsIncludes(dep);
	std::vector<std::filesystem::path> targetsLib = Package::dependencyTargetsLib(dep);

	if (dep.type == PackageType::Composed) {
		// composed dependencies treated differently
		// they might have multiple include/lib dirs
		// so their "to" destination is grouped in a directory dep.name

		for (std::filesystem::path& path: targetsInclude) {
			DependencyTarget target;
			target.from = path;
			target.to = utils::fs::extendPath<4>(pkgPath, { "includes", "src", dep.name.c_str(), path.filename().c_str() });
			targets.push_back(target);
		}

		for (std::filesystem::path& path: targetsLib) {
			DependencyTarget target;
			target.from = path;
			target.to = utils::fs::extendPath<4>(pkgPath, { "includes", "lib", dep.name.c_str(), path.filename().c_str() });
			targets.push_back(target);
		}

		return targets;
	}

	// non-composed packages, whether managed or non-managed, behave the same
	// while at the moment they always have a single include/lib dir
	// we still loop as that might change in the future
	for (std::filesystem::path& path: targetsInclude) {
		DependencyTarget target;
		target.from = path;
		target.to = utils::fs::extendPath<3>(pkgPath, { "includes", "src", dep.name.c_str() });
		targets.push_back(target);
	}

	for (std::filesystem::path& path: targetsLib) {
		DependencyTarget target;
		target.from = path;
		target.to = utils::fs::extendPath<3>(pkgPath, { "includes", "lib", dep.name.c_str() });
		targets.push_back(target);
	}

	return targets;
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
				PrintNice::error(
					fmt::format("Package {} missing in chain", depName),
					ErrorSeverity::Medium
				);
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

void Package::materializeDependencies(const Package &pkg, const Package& entry)
{

	if (!entry.managed) {
		// entry must be a managed package
		return;
	}

	if (entry.dependencies.size() == 0 && entry.uses.size() == 0) {
		// nothing to do
		return;
	}

	if (pkg.name == entry.name) {
		// entry point
		PrintNice::print(
			("Materializing package " + entry.name + "...").c_str(),
			OutputType::Normal,
			TextStyle::Italic
		);

		// get all dependencies, including transient, and materialize them recursively
		std::unordered_set<std::string> deps = Package::getDependenciesDeep(entry);

		for (const std::string& depName: deps) {
			MaybePackage dep = Package::get(depName.c_str());
			if (std::holds_alternative<PackageNotFound>(dep)) {
				
				PrintNice::error("Missing dependency " + depName + ", skipped");
				continue;
			}
			// materialize dependency
			Package::materializeDependencies(std::get<Package>(dep), entry);
		}

		PrintNice::success("✓ Package " + pkg.name + " materialized");

		return;
	}

	// materialize the dependency
	PrintNice::print(
		("Materializing dependency " + pkg.name + "...").c_str(),
		OutputType::Normal,
		TextStyle::Italic
	);
	Package::materializeDependencies(pkg, pkg);

	PrintNice::print(
		("Dependency " + pkg.name + " materialized").c_str(),
		OutputType::Info,
		TextStyle::Italic
	);

	// copy materialized dependency to entry Package
	PrintNice::print(
		("Copying materialized " + pkg.name + " to " + entry.name + "...").c_str(),
		OutputType::Info,
		TextStyle::Italic
	);
	std::vector<DependencyTarget> targets = Package::dependencyTargets(entry, pkg);

	for (auto& target: targets) {
		if (std::filesystem::exists(target.to)) {
			std::filesystem::remove_all(target.to);
		}

		std::filesystem::copy(
			target.from,
			target.to,
			std::filesystem::copy_options::recursive
		);
	}

	PrintNice::print(
		("Copied materialized " + pkg.name + " to " + entry.name + ", unmaterializing " + pkg.name).c_str(),
		OutputType::Info,
		TextStyle::Italic
	);
	PrintNice::print();

	// unmaterialize dependency
	Package::unmaterializeDependencies(pkg);
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
		PrintNice::error(
			"Can't unmaterialize dependency of " + pkg.name + ", " + depName + ", not found. Skipped."
		);
	}

	PrintNice::success("Package " + pkg.name + " dependencies unmaterialized");
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

std::vector<std::string> Package::listLinkable(const Package &pkg)
{
	std::vector<std::string> linkable;

	// include uses in links
	for (const std::string& use: pkg.uses) {
		std::string command = "pkg-config --libs-only-l ";
		command += use;

		std::string result = "";
		try {
			result = utils::system::runCommandOutput(command);
		} catch(std::runtime_error e) {
			PrintNice::error("Skipped " + use);
			continue;
		}

		std::vector<std::string> items = utils::string::split(result, " ");

		for (std::string& item: items) {
			// items are prepended with "-l"
			std::string trimmed = utils::string::trim(item);
			if (trimmed.size() < 3) {continue;}
			linkable.push_back(trimmed.substr(2));
		}
	}

	std::vector<Package> dependencies = Package::getDependencies(pkg);

	for (Package& dep: dependencies) {

		if (dep.managed && Package::isLibrary(dep)) {
			// managed library, include it's name
			linkable.push_back(dep.name);
		}

		for (std::string& obj: dep.linkableObjects) {
			// direct dependency obj
			if (dep.type == PackageType::Composed) {
				// use raw link for composed package dependencies
				linkable.push_back(obj);
			} else {
				// obj contains a full path to obj, extract name using Package::linkableObject
				std::string objName = Package::linkableObject(std::filesystem::path(obj));
				linkable.push_back(objName);
			}
		}
		
		// transient recursive
		std::vector<std::string> objTransient = Package::listLinkable(dep);
		linkable.insert(linkable.end(), objTransient.begin(), objTransient.end());
	}

	return linkable;
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

bool Package::build(const Package &pkg, BuildConfig conf, const char* target)
{
	if (!Package::generateCmake(pkg)) {
		return false;
	}

	// build
	std::string command =
		"cd " + pkg.path + " && " +
		"make config=" + (conf == BuildConfig::Debug ? "debug" : "release");

	if (target != nullptr) {
		command += ' ';
		command += target;
	} else {
		if (conf == BuildConfig::Release) {
			// don't build tests in release mode
			command += ' ';
			command += pkg.name;
		}
	}

	bool success = utils::system::runCommand(command) == 0;

	if (success) {
		if (conf == BuildConfig::Debug) {
			PrintNice::success(fmt::format("✓ Successfully built debug binaries for {}", pkg.name));
			auto path = Package::getPath<3>(pkg, { "bin", "Debug", pkg.name.c_str() });
			PrintNice::print(
				path.c_str(),
				OutputType::Normal,
				TextStyle::Italic
			);
		} else {
			PrintNice::success(fmt::format("✓ Successfully built release binary for {}", pkg.name));
			auto path = Package::getPath<3>(pkg, { "bin", "Release", pkg.name.c_str() });
			PrintNice::print(
				path.c_str(),
				OutputType::Normal,
				TextStyle::Italic
			);
		}
	}

	return success;
}

bool Package::check(const Package &pkg, bool strict)
{
	auto stream = PrintNice::stream();
	const char* pkgType = pkg.managed ? "managed" : "non-managed";
	stream << fmt::format("Checking {} package", pkgType).c_str() <<
	TextStyledToken{
		pkg.name.c_str(),
		OutputType::Info,
		0
	} <<
	StreamOut();

	auto path = Package::getPath(pkg);
	return Package::checkPath(path, pkg.managed, strict);
}

bool Package::checkPath(const std::filesystem::path& path, bool asManaged, bool strict)
{
	bool existsOnFilesystem = std::filesystem::exists(path);
	
	if (!existsOnFilesystem) {
		PrintNice::error(fmt::format("Not found in {}", path.c_str()), ErrorSeverity::Medium);
		return false;
	}
	
	if (!asManaged) {
		// non managed packages only need to exist in the file system
		PrintNice::print("✓ Package valid", OutputType::Success);
		return true;
	}
	
	// managed package checks
	
	// check for /src
	auto srcPath = utils::fs::extendPath<1>(path, { "src" });
	if (!std::filesystem::exists(srcPath)) {
		PrintNice::error("Package is missing it's /src directory", ErrorSeverity::Medium);
		return false;
	}
	
	// check for /includes
	auto includesPath = utils::fs::extendPath<1>(path, { "includes" });
	if (!std::filesystem::exists(includesPath)) {
		PrintNice::error("Package is missing it's /includes directory", ErrorSeverity::Medium);
		return false;
	}
	
	// check for /includes/src
	auto includesSrcPath = utils::fs::extendPath<2>(path, { "includes", "src" });
	if (!std::filesystem::exists(includesSrcPath)) {
		PrintNice::error("Package is missing it's /includes/src directory", ErrorSeverity::Low);
		return false;
	}
	
	// check for /includes/lib
	auto includesLibPath = utils::fs::extendPath<2>(path, { "includes", "lib" });
	if (!std::filesystem::exists(includesLibPath)) {
		PrintNice::error("Package is missing it's /includes/lib directory", ErrorSeverity::Low);
		return false;
	}
	
	if (strict) {
		// check for /tests
		auto includesTestsPath = utils::fs::extendPath<1>(path, { "tests" });
		if (!std::filesystem::exists(includesTestsPath)) {
			PrintNice::error("Package is missing it's /tests directory", ErrorSeverity::Low);
			return false;
		}

		// strict mode checks for files that can be generated, so they don't have to exist
		// in strict mode, must contain premake5.lua
		auto luaPath = utils::fs::extendPath<1>(path, { "premake5.lua" });
		if (!std::filesystem::exists(luaPath)) {
			PrintNice::print("Package is missing premake5.lua", OutputType::Warning);
			return false;
		}
	}
	
	PrintNice::print("✓ Package valid", OutputType::Success);
	
	return true;
}

void Package::push(const Package &pkg)
{
	// check if .git directory exists
	const auto gitDir = Package::getPath<1>(pkg, { ".git" });
	if (!std::filesystem::exists(gitDir)) {
		PrintNice::error(".git directory not found within package directory, aborted.");
		return;
	}

	const auto pkgDir = Package::getPath(pkg);

	// materialize dependencies so the package is portable
	Package::materializeDependencies(pkg, pkg);

	// get value of CPPM_ENABLE_GIT
	// user may have set it to "1", we want to restore it to what it was later
	const char* userEnableGitValue = getenv("CPPM_ENABLE_GIT") == nullptr ? "0" : getenv("CPPM_ENABLE_GIT");

	// enable 'git push'
	setenv("CPPM_ENABLE_GIT", "1", 1);

	// track includes
	Package::gitSetTrackIncludes(pkg, true, Empty());

	// stage ./includes
	utils::system::runCommand("cd " + pkgDir.string() + " && git add -f includes/");

	PrintNice::info("Checking dependencies for changes");

	// check if there are changes in staged ./includes
	bool dependenciesChanged = utils::system::runCommand("cd " + pkgDir.string() + " && git diff --staged --exit-code --quiet includes/") != 0;

	if (dependenciesChanged) {
		// dependencies changed, commit
		PrintNice::info("Dependencies changed and will be committed");
		PrintNice::info(
			"Enter commit message for dependency changes or leave blank to use \"Dependency changes\":"
		);

		// allow user to specify the commit message for dependency changes
		std::string commitMessage = "";
		std::getline(std::cin, commitMessage);

		if (utils::string::trim(commitMessage) == "") {
			commitMessage = "Dependency changes";
		}

		utils::system::runCommand("cd " + pkgDir.string() + " && git commit -m \""+ commitMessage +"\"");

		// push to remote
		utils::system::runCommand("cd " + pkgDir.string() + " && CPPM_ENABLE_GIT=1 git push origin main");
		
	} else {
		// no dependency changes, unstage
		PrintNice::info("Dependencies unchanged");
		utils::system::runCommand("cd " + pkgDir.string() + " && git reset includes/");
	}

	// untrack includes
	Package::gitSetTrackIncludes(pkg, false, Empty());

	// unmaterialize
	Package::unmaterializeDependencies(pkg);

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
	int checked = 0;
	int valid = 0;
	bool foundInvalid = false;
	std::vector<Package> packages = Package::packages();
	for (const Package& pkg: packages) {
		bool isValid = Package::check(pkg, true);
		checked++;
		valid += isValid ? 1 : 0;
		if (!isValid) {
			foundInvalid = true;
		}
		PrintNice::print();
	}

	auto stream = PrintNice::stream();
	std::string msg = fmt::format("Checked {} packages,", checked);
	std::string msgValid = fmt::format("{} valid", valid);
	std::string msgInvalid = fmt::format("{} invalid", checked - valid);

	stream << msg.c_str();

	if (valid > 0) {
		stream << TextStyledToken{
			msgValid.c_str(),
			OutputType::Success,
			0
		};
	}

	if (valid < checked) {
		stream << TextStyledToken{
			msgInvalid.c_str(),
			OutputType::Error,
			0
		};
	}

	stream << StreamOut();

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
		utils::fs::extendPath<2>(path, { "includes", "lib" }),
		utils::fs::extendPath<1>(path, { "tests" })
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
	PrintNice::print("Generating cmake...", OutputType::Info);
	return utils::system::runCommand("cd " + pkg.path + " && premake5 gmake > /dev/null") == 0;
}

void Package::generateClangCompileCommands(const Package& pkg)
{
	if (!pkg.managed) {
		PrintNice::warning("Can only be done within a managed package");
		return;
	}

	auto path = Package::getPath(pkg);

	// find all .cpp files in src
	std::vector<std::filesystem::path> cppFiles = utils::fs::filterRecursive(
		utils::fs::extendPath<1>(path, { "src" }),
		[](const std::filesystem::path& p) {
			return p.extension() == ".cpp";
		}
	);

	// output array containing configurations for all cpp files
	boost::json::array output;

	// find all directories in ./src
	std::vector<std::string> includedirs = utils::vector::map<std::string, std::filesystem::path>(
		utils::fs::filterRecursive(
			utils::fs::extendPath<1>(path, { "src" }),
			[](const std::filesystem::path& p) {
				return std::filesystem::is_directory(p);
			}
		),
		[](const std::filesystem::path& p) {
			return "-I" + p.string();
		}
	);

	// find all directories in ./includes
	std::vector<std::string> includedirsIncludes = utils::vector::map<std::string, std::filesystem::path>(
		utils::fs::filterRecursive(
			utils::fs::extendPath<1>(path, { "includes" }),
			[](const std::filesystem::path& p) {
				return std::filesystem::is_directory(p);
			}
		),
		[](const std::filesystem::path& p) {
			return "-I" + p.string();
		}
	);

	// join the src and includes directories
	includedirs.insert(includedirs.end(), includedirsIncludes.begin(), includedirsIncludes.end());
	
	// same command for all files
	std::string command = "cc -MD -MP -DDEBUG " + utils::vector::join(includedirs, " ");

	// include used system packages
	for (const std::string& used: pkg.uses) {
		command += " " + utils::system::runCommandOutput("pkg-config --cflags-only-I " + used);
	}

	for (const std::filesystem::path& cpp: cppFiles) {
		boost::json::object conf;

		conf["directory"] = path.string();
		conf["file"] = cpp.string();
		conf["command"] = command;
		
		output.push_back(conf);
	}

	// write to compile_commands.json
	utils::json::write(
		utils::fs::extendPath<1>(path, { "compile_commands.json" }),
		output
	);

	PrintNice::success("Generated clang config in compile_commands.json");
}

bool Package::isLibrary(const Package &pkg)
{
	return pkg.type == PackageType::StaticLib || pkg.type == PackageType::SharedLib;
}

void Package::generatePremake(const Package &pkg)
{
	std::filesystem::path premakePath = Package::getPath<1>(pkg, { "premake5.lua" });
	std::ofstream fstream(premakePath);

	if (!fstream.is_open()) {
		std::cerr << "Could not open premake file for writing, path: " << premakePath.string() << std::endl;
		return;
	}

	// workspace
	fstream << "workspace \"" << pkg.name << '"' << std::endl; //variable
	fstream << "\tconfigurations { \"Debug\", \"Release\" }" << std::endl << std::endl;

	// project
	std::vector<std::string> linked = Package::listLinkable(pkg);
	fstream << Package::premakeProject(
		pkg.name,
		pkg.type,
		"bin/%{cfg.buildcfg}",
		{ "./src/**.h", "./src/**.cpp" },
		{ "./includes/src", "./includes/src/**", "./includes/uses/src/**" },
		{ "./includes/lib/**", "./includes/uses/lib/**" },
		pkg.uses,
		linked
	);

	// if project is not a library, but has at least one test
	// generate a static lib binary for it so it can be used within tests
	bool testLib = !Package::isLibrary(pkg) && pkg.tests.size() > 0;
	const char* testLibDir = "./tests/lib";
	if (testLib) {
		fstream << Package::premakeProject(
			"testlib-" + pkg.name,
			Package::typeFromString("StaticLib"),
			testLibDir,
			{ "./src/**.h", "./src/**.cpp" },
			{ "./includes/src", "./includes/src/**", "./includes/uses/src/**" },
			{ "./includes/lib/**", "./includes/uses/lib/**" },
			pkg.uses,
			linked
		);
	}


	std::vector<std::string> testsLibdirs({ "./includes/lib/**" });
	if (testLib) {
		testsLibdirs.push_back(testLibDir);
	}

	// tests projects
	std::vector<std::string> linksTests;
	linksTests.push_back(pkg.name);

	if (testLib) {
		linksTests.push_back("testlib-" + pkg.name);
	}

	linksTests.insert(linksTests.end(), linked.begin(), linked.end());

	for (const std::string& testName: pkg.tests) {
		fstream << Package::premakeProject(
			"Test-" + testName,
			Package::typeFromString("ConsoleApp"),
			"./tests/bin",
			{ "./tests/" + testName + ".cpp" },
			{ "./includes/src", "./includes/src/**", "./src", "./src/**" },
			testsLibdirs,
			pkg.uses,
			linksTests
		);
	}

	// done, flush and close
	fstream.flush();
	fstream.close();
}

std::string Package::premakeProject(
	const std::string& name,
	PackageType kind,
	const char* targetdir,
	std::vector<std::string> files,
	std::vector<std::string> includedirs,
	std::vector<std::string> libdirs,
	std::vector<std::string> uses,
	std::vector<std::string> links
)
{
	std::stringstream output;
	
	output << "project \"" << name << '"' << std::endl;
	output << "\tlanguage \"C++\"" << std::endl;
	output << "\tkind \"" << Package::typeToString(kind) << '"' << std::endl;
	output << "\tcppdialect \"C++20\"" << std::endl;
	output << "\tarchitecture \"x64\"" << std::endl;
	output << "\ttargetdir \"" << targetdir << "\"" << std::endl;
	output << "\tfiles { " << utils::vector::toQuotedList(files) << " }" << std::endl;
	output << "\tincludedirs { " << utils::vector::toQuotedList(includedirs) << " }" << std::endl;
	output << "\tlibdirs { " << utils::vector::toQuotedList(libdirs) << " }" << std::endl;

	if (uses.size() > 0) {
		// uses system libraries
		output << "\tbuildoptions {" << std::endl;
		output << utils::vector::join(
			utils::vector::map<std::string, std::string>(uses, [](const std::string& str) {
				return "\t\t\"`pkg-config --cflags " + str + "`\"";
			}),
			", "
		);
		output << "\t}" << std::endl;

		output << "\tlinkoptions {" << std::endl;
		output << utils::vector::join(
			utils::vector::map<std::string, std::string>(uses, [](const std::string& str) {
				return "\t\t\"`pkg-config --libs " + str + "`\"";
			}),
			", "
		);
		output << "\t}" << std::endl;
	}

	output << "\tlinks {" << std::endl;
	output << "\t\t" << utils::vector::toQuotedList(links) << std::endl;
	output << "\t}" << std::endl;

	// filters
	output << "\tfilter \"configurations:Debug\"" << std::endl;
	output << "\t\tdefines { \"DEBUG\" }" << std::endl;
	output << "\t\tsymbols \"On\"" << std::endl;

	output << "\tfilter \"configurations:Release\"" << std::endl;
	output << "\t\tdefines { \"NDEBUG\" }" << std::endl;
	output << "\t\toptimize \"On\"" << std::endl;
	
	return output.str();
}

MaybePackage Package::includesPath(std::filesystem::path p)
{
	const std::string pathString = p.string();
	return Package::find([&pathString](Package& pkg) {
		return pathString == pkg.path || pathString.starts_with(pkg.path + "/");
	});
}

void Package::generateVSC(const Package &pkg)
{
	boost::json::object root;
	boost::json::array configurations;
	boost::json::object configuration;
	boost::json::array includePaths;

	// allow includes from current project
	includePaths.push_back("${workspaceFolder}/src/**/*");
	includePaths.push_back("${workspaceFolder}/includes/src/**/*");

	std::unordered_set<std::string> useDirs = Package::useIncludeDirs(pkg);
	for (auto& useDir: useDirs) {
		includePaths.push_back(useDir.c_str());
	}

	configuration["name"] = pkg.name;
	configuration["includePath"] = includePaths;
	configuration["defines"] = boost::json::array({});
	configuration["cppStandard"] = "c++20";

	configurations.push_back(configuration);

	root["configurations"] = configurations;
	root["version"] = 4;

	if (!Package::usesVSC(pkg)) {
		std::filesystem::path vscDir = Package::getPath<1>(pkg, { ".vscode" });
		std::filesystem::create_directory(vscDir);
	}

	std::filesystem::path configPath = Package::getPath<2>(pkg, { ".vscode", "c_cpp_properties.json" });

	utils::json::write(configPath, root);

	PrintNice::success(fmt::format("Generated VSC configuration in {}", configPath.string()));
}

void Package::generateVSCDebugConf(const Package &pkg, std::vector<std::string> &args)
{
	// tasks.json
	boost::json::object tasksRoot;
	boost::json::array tasks;
	boost::json::object taskBuild;
	boost::json::object taskBuildOptions;
	boost::json::object taskBuildGroup;

	taskBuildOptions["cwd"] = "${workspaceFolder}";

	taskBuildGroup["kind"] = "build";
	taskBuildGroup["isDefault"] = true;

	taskBuild["type"] = "shell";
	taskBuild["label"] = "Build with cppm";
	taskBuild["command"] = "cppm build";
	taskBuild["options"] = taskBuildOptions;
	taskBuild["group"] = taskBuildGroup;

	tasks.push_back(taskBuild);

	tasksRoot["version"] = "2.0.0";
	tasksRoot["tasks"] = tasks;

	// launch.json
	boost::json::object launchRoot;
	boost::json::array launchConfigurations;
	boost::json::object launchConfiguration;
	boost::json::object prettyPrint;
	boost::json::array launchArgs;
	boost::json::array envVars; // in the future this could be configured by the user
	boost::json::object envPath;
	boost::json::array setupCommands;


	// fill args array
	for (std::string arg: args) {
		launchArgs.push_back(arg.c_str());
	}

	envPath["name"] = "PATH";
	envPath["value"] = "${env:PATH}";
	envVars.push_back(envPath);

	prettyPrint["description"] = "Enable pretty-printing for gdb";
	prettyPrint["text"] = "-enable-pretty-printing";
	prettyPrint["ignoreFailures"] = true;
	setupCommands.push_back(prettyPrint);
	
	launchConfiguration["name"] = "Debug";
	launchConfiguration["type"] = "cppdbg";
	launchConfiguration["request"] = "launch";
	launchConfiguration["program"] = "${workspaceFolder}/bin/Debug/" + pkg.name;
	launchConfiguration["args"] = launchArgs;
	launchConfiguration["stopAtEntry"] = false;
	launchConfiguration["cwd"] = "${workspaceFolder}";
	launchConfiguration["environment"] = envVars;
	launchConfiguration["MIMode"] = "gdb";
	launchConfiguration["miDebuggerPath"] = "/usr/bin/gdb";
	launchConfiguration["setupCommands"] = setupCommands;
	launchConfiguration["preLaunchTask"] = "Build with cppm";

	launchConfigurations.push_back(launchConfiguration);

	launchRoot["version"] = "2.0.0";
	launchRoot["configurations"] = launchConfigurations;

	// create .vscode directory if needed
	if (!Package::usesVSC(pkg)) {
		std::filesystem::path vscDir = Package::getPath<1>(pkg, { ".vscode" });
		std::filesystem::create_directory(vscDir);
	}

	std::filesystem::path pathTasks = Package::getPath<2>(pkg, { ".vscode", "tasks.json" });
	std::filesystem::path pathLaunch = Package::getPath<2>(pkg, { ".vscode", "launch.json" });

	utils::json::write(pathTasks, tasksRoot);
	utils::json::write(pathLaunch, launchRoot);

	PrintNice::success("VSC debug configuration generated, you can launch you app in debug mode from \"Run and Debug\" (Ctrl+Shift+D)");
	PrintNice::print("Add breakpoints in VSC and run the debugger. If the debugger does not run, add following to .vscode/settings.json:");
	PrintNice::print(R"(
"terminal.integrated.automationProfile.linux": {
    "path": "/bin/bash",
    "args": ["-i"]
})", OutputType::Normal, TextStyle::Italic);

	PrintNice::info("To configure debugger to start with a different set of arguments, re run this command with new argument list");
}

bool Package::usesVSC(const Package &pkg)
{
	std::filesystem::path vscDir = Package::getPath<1>(pkg, { ".vscode" });
	return std::filesystem::exists(vscDir);
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
	PrintNice::error(
		fmt::format("Can't update package registry, package {} not registered", pkg.name),
		ErrorSeverity::Medium
	);
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
			f << "#include " << '"' << hName << '"' << '\n' << std::endl;
		}
		f.close();
		added = true;
	} else {
		PrintNice::warning(cppName + " exists, skipping");
	}

	if (!std::filesystem::exists(hPath)) {
		std::ofstream f(hPath);
		if (f.is_open()) {
			f << "#pragma once\n" << std::endl;
		}
		f.close();
		added = true;
	} else {
		PrintNice::warning(hName + " exists, skipping");
	}

	// re-generate premake
	Package::generateCmake(pkg);

	if (added) {
		PrintNice::success("Source " + srcName + " added");
	} else {
		PrintNice::info("No files were created");
	}
}

void Package::addSrc(const char *const pkgName, const char *srcName)
{
	MaybePackage pkg = Package::get(pkgName);

	if (std::holds_alternative<PackageNotFound>(pkg)) {
		return PrintNice::warning(std::string("Package ") + pkgName + " does not exist");
	}

	Package::addSrc(std::get<Package>(pkg), srcName);
}

std::string Package::typeToString(PackageType t) {
	switch (t) {
		case PackageType::ConsoleApp: return "ConsoleApp";
		case PackageType::WindowedApp: return "WindowedApp";
		case PackageType::StaticLib: return "StaticLib";
		case PackageType::SharedLib: return "SharedLib";
		case PackageType::Composed: return "Composed";
	}
	return "";
}

void Package::listDependencies(const Package &pkg)
{
	const auto dependencies = Package::getDependencies(pkg);
	if (dependencies.size() == 0 && pkg.uses.size() == 0) {
		PrintNice::print("No dependencies", OutputType::Normal, TextStyle::Italic);
	}
	if (dependencies.size() > 0) {
		PrintNice::print("📁 Dependencies:");
		for (const Package& dep: dependencies) {
			auto stream = PrintNice::stream();
			stream << "|-" << Package::managedIndicator(dep.managed) << dep.name.c_str() << StreamOut();
		}
	}

	if (pkg.managed) {
		// list used system libs
		PrintNice::print("🔧 Uses:");
		for (const std::string& use: pkg.uses) {
			auto stream = PrintNice::stream();
			stream << "|-" << use.c_str() << StreamOut();
		}
	}
}

void Package::listDependents(const Package &pkg)
{
	const std::vector<Package> dependents = Package::dependents(pkg.name.c_str());

	if (dependents.size() == 0) {
		PrintNice::print("No dependents", OutputType::Normal, TextStyle::Italic);
	} else {
		PrintNice::print("📁 Dependents:");
		for (const Package& dep: dependents) {
			auto stream = PrintNice::stream();
			stream <<
				"|-" << Package::managedIndicator(dep.managed) << dep.name.c_str() << StreamOut();
		}
	}
}

void Package::vcpkgRegister(const char* pkgName)
{

	std::string vcpkgDir = utils::system::appDataDir() + std::filesystem::path::preferred_separator + ".vcpkg";

	if (!std::filesystem::exists(vcpkgDir)) {
		PrintNice::warning(fmt::format("Is vcpkg installed? Did not find {}", vcpkgDir));
		return;
	}

	// command to list dependencies
	std::string command = "vcpkg depend-info ";
	command += pkgName;
	command += " 2>&1";

	// read output of vcpkg depend-info into a string
	std::string result;
	try {
		result = utils::system::runCommandOutput(command);
	} catch (std::runtime_error e) {
		PrintNice::error(e.what(), ErrorSeverity::Low);
		return;
	}

	// split at newline char
	std::vector<std::string> lines = utils::string::split(result, "\n");

	// split each line at ":"
	std::vector<std::vector<std::string>> packagesWithDepstring;
	
	std::transform(
		lines.begin(),
		lines.end(),
		std::back_inserter(packagesWithDepstring),
		[](std::string& line) {
			return utils::string::split(line, ":");
		}
	);
	
	
	for (auto pkg: packagesWithDepstring) {
		if (pkg.size() > 1) {
			std::string packageName = pkg[0];
			std::string depsString = pkg[1];

			// remove [...] from packageName
			const auto end = packageName.find('[');
			if (end != std::string::npos) {
				packageName = packageName.substr(0, end);
			}

			auto pkgPath = Package::vcpkgPackagePath(packageName);

			if (std::holds_alternative<Empty>(pkgPath)) {
				PrintNice::error(
					fmt::format("Could not find vcpkg package path for {}", packageName),
					ErrorSeverity::Medium
				);
				continue;
			}

			
			Package::registerPackage(std::get<std::filesystem::path>(pkgPath), false, packageName.c_str());
			MaybePackage pkg = Package::get(packageName.c_str());

			if (std::holds_alternative<PackageNotFound>(pkg)) {
				PrintNice::error(
					fmt::format("Package {} expected to be registered at this point", packageName),
					ErrorSeverity::Medium
				);
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
					PrintNice::error(
						fmt::format("Dependency of {}, {} expected to be registered at this point", packageName, depnameClean),
						ErrorSeverity::Medium
					);
					continue;
				}

				Package::addDependency(std::get<Package>(pkg), depnameClean.c_str());
			}

		}
	}

}

void Package::display(const Package& pkg)
{
	PrintStream stream = PrintNice::stream();

	stream.separator = "";

	stream <<
		// managed status indicator
		Package::managedIndicator(pkg.managed)  << " " <<

		"Package name: " <<

		TextStyledToken{
			pkg.name.c_str(),
			OutputType::Info,
			0
		} << "\n" <<

		"  Type: " <<

		TextStyledToken{
			(pkg.managed ? "managed" : "non-managed"),
			OutputType::Info,
			0
		} << " " <<
		
		TextStyledToken{
			Package::typeToString(pkg.type).c_str(),
			OutputType::Info,
			0
		} << "\n" <<

		"  Version: " <<
		
		TextStyledToken{
			pkg.version.c_str(),
			OutputType::Info,
			0
		} << "\n" <<

		StreamOut();

	if (pkg.type == PackageType::Composed) {
		// composed packages don't have a single path
		// show includes/lib paths
		
		// includeDirs
		stream << " 📁 Includes:\n";

		for (const std::string& path: pkg.includeDirs) {
			stream << "  |-" <<
			
			TextStyledToken{
				path.c_str(),
				OutputType::Info,
				0
			}

			<< "\n";
		}

		stream << StreamOut();

		// libDirs
		stream << " 📁 Libs:\n";

		for (const std::string& path: pkg.libDirs) {
			stream << "  |-" <<
			
			TextStyledToken{
				path.c_str(),
				OutputType::Info,
				0
			}

			<< "\n";
		}

		stream << StreamOut();

		// links
		stream << " 🔗 Links:\n";

		for (const std::string& path: pkg.linkableObjects) {
			stream << "  |-" <<
			
			TextStyledToken{
				path.c_str(),
				OutputType::Info,
				0
			}

			<< "\n";
		}

		stream << StreamOut();

	} else {
		stream << "  Path: " <<
		
		TextStyledToken{
			pkg.path.c_str(),
			OutputType::Info,
			0
		} << StreamOut();
	}
		
	PrintNice::print();
	
	Package::listDependencies(pkg);
	PrintNice::print();

	Package::listDependents(pkg);

	if (Package::isLibrary(pkg) && pkg.linkableObjects.size() > 0) {
		// show linkable objects
		std::cout << "Linkable objects:" << std::endl;
		for (const std::string& objName: pkg.linkableObjects) {
			std::cout << "|- " << objName << std::endl;
		}
	}

	if (pkg.managed) {
		PrintNice::print();
		if (pkg.tests.size() == 0) {
			PrintNice::print("No tests", OutputType::Normal, TextStyle::Italic);
		} else {
			PrintNice::print("Tests:", OutputType::Normal, TextStyle::Bold);
			for (const std::string& test: pkg.tests) {
				PrintNice::print(test.c_str());
			}
		}
	}
}

void Package::display(const char *const name)
{
	MaybePackage pkg = Package::get(name);

	if (std::holds_alternative<PackageNotFound>(pkg)) {
		PrintNice::print(std::string("Package ") + name + " does not exist", OutputType::Warning);
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
	if (strcmp(t, "Composed") == 0) {
		return PackageType::Composed;
	}

	return PackageType::ConsoleApp;
}

Package Package::fromJSON(PackageJSON data)
{
	return Package(
		data.at("name").as_string().c_str(),
		data.at("path").as_string().c_str(),
		Package::typeFromString(data.at("type").as_string().c_str()),
		data.at("version").as_string().c_str(),
		utils::json::extractArrayFromJSONObject(data, "includeDirs"),
		utils::json::extractArrayFromJSONObject(data, "libDirs"),
		utils::json::extractArrayFromJSONObject(data, "linkableObjects"),
		utils::json::extractArrayFromJSONObject(data, "dependencies"),
		utils::json::extractArrayFromJSONObject(data, "uses"),
		utils::json::extractArrayFromJSONObject(data, "tests"),
		data.at("managed").as_bool(),
		data.at("registeredAt").as_int64()
	);
}

PackageJSON Package::toJSON(Package &pkg)
{
	PackageJSON json;
	json["name"] = pkg.name;
	json["path"] = pkg.path;
	json["type"] = Package::typeToString(pkg.type);
	json["version"] = pkg.version;
	json["managed"] = pkg.managed;
	json["registeredAt"] = pkg.registeredAt;
	json["linkableObjects"] = utils::json::toJSONArray(pkg.linkableObjects);
	json["includeDirs"] = utils::json::toJSONArray(pkg.includeDirs);
	json["libDirs"] = utils::json::toJSONArray(pkg.libDirs);
	json["dependencies"] = utils::json::toJSONArray(pkg.dependencies);
	json["uses"] = utils::json::toJSONArray(pkg.uses);
	json["tests"] = utils::json::toJSONArray(pkg.tests);
	return json;
}