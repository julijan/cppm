#pragma once

#include <vector>

struct HelpArguments {
	const char* name;
	const char* description;
	bool required;
	bool list;
	bool literal;
};

struct HelpItem {
	const char* command;
	const char* alias;
	const char* description; 
	const char* descriptionShort; 
	const std::vector<HelpArguments> arguments;
};

inline const std::vector<HelpItem> HELP_ITEMS = {
	{
		.command = "init",
		.alias = "",
		.description = "Creates a managed package in the current directory. Packages are the unit in which cppm manages C++ projects. cppm keeps a system-wide registry of packages allowing you to easily include them in other packages. Package name is it's unique identifier - there can't be 2 packages with the same name. There are two types of packages: managed and non-managed. Managed packages are your own projects and libraries, non-managed are 3rd party packages which you can easily include in your managed packages, to register a 3rd party code as a package use cppm register, for details run cppm help register.",
		.descriptionShort = "Create a new managed package",
		.arguments = {
			{
				.name = "packageName",
				.description = "unique identifier for your managed package",
				.required = true,
				.list = false,
				.literal = false
			}
		}
	},
	{
		.command = "add",
		.alias = "",
		.description = "When issued within a package ./src/[srcName].cpp and ./src/[srcName].h will be created, and cmake will be re-generated to include the new sources in the build. This is the preferred method to create new source files.",
		.descriptionShort = "Add new source file(s)",
		.arguments = {
			{
				.name = "srcName",
				.description = "file name(s) to create",
				.required = true,
				.list = true,
				.literal = false
			}
		}
	},
	{
		.command = "list",
		.alias = "ls",
		.description = "List all registered packages",
		.descriptionShort = "List all registered packages",
		.arguments = {}
	},
	{
		.command = "show",
		.alias = "",
		.description = "Show package details, which include:\nPackage name: string\nVersion: string\nType: PackageType\nPath: string\nDependencies: list\nDependents: list\n\nProjectType: ConsoleApp | WindowedApp | StaticLib | SharedLib",
		.descriptionShort = "Show package details",
		.arguments = {
			{
				.name = "packageName",
				.description = "Required unless command is executed in a package directory, in which case description for current package is shown",
				.required = false,
				.list = false,
				.literal = false
			}
		}
	},
	{
		.command = "include",
		.alias = "",
		.description = "Include given package as a dependency for current package.",
		.descriptionShort = "Include package as a dependency",
		.arguments = {
			{
				.name = "packageName",
				.description = "Package(s) to be included",
				.required = true,
				.list = true,
				.literal = false
			}
		}
	},
	{
		.command = "exclude",
		.alias = "",
		.description = "Exclude previously included package. Removes a dependency.",
		.descriptionShort = "Exclude package as a dependency",
		.arguments = {
			{
				.name = "packageName",
				.description = "Package(s) to be excluded",
				.required = true,
				.list = true,
				.literal = false
			}
		}
	},
	{
		.command = "register",
		.alias = "",
		.description = "Register current directory as a package. By default, package is assumed to be 3rd party code (a non-managed package), if you want to turn one of your existing codebases into a managed package, you can specify \"managed\" argument. Once package is registered, you can use it from your packages using include command (cppm help include). You will be able to assign a name to package when you run the command, assumed name is the directory name. Good example for using this without managed argument is if you use vcpkg or other package managers, you can register the installed packages, and include them with a single command.",
		.descriptionShort = "Register current directory as a package",
		.arguments = {
			{
				.name = "managed",
				.description = "Register as managed package",
				.required = false,
				.list = false,
				.literal = true
			}
		}
	},
	{
		.command = "unregister",
		.alias = "",
		.description = "Remove a package from the registry. If none of your packages depend on it, it will simply remove it from the registry, otherwise you will be prompted whether you want to proceed.",
		.descriptionShort = "Remove a package from the registry",
		.arguments = {
			{
				.name = "packageName",
				.description = "Package name to unregister. Required, unless you are within the package directory in which case, if ommitted, current package is assumed",
				.required = false,
				.list = true,
				.literal = false
			}
		}
	},
	{
		.command = "move",
		.alias = "mv",
		.description = "Safely move package files to a new destination in the filesystem. Safely means that package dependents will not be broken after the move, as opposed to moving the package in other way.",
		.descriptionShort = "Safely move package files to a new destination in the filesystem",
		.arguments = {
			{
				.name = "packageName",
				.description = "Package to be moved. Required, unless you are within the package directory in which case current package is assumed",
				.required = false,
				.list = false,
				.literal = false
			},
			{
				.name = "pathTo",
				.description = "Path to move the package directory into. It should not include the package directory name. Assume you have package in /a and want to move it to /b, cppm mv /b/ will result with /b/a",
				.required = true,
				.list = false,
				.literal = false
			}
		}
	},
	{
		.command = "vsc",
		.alias = "",
		.description = "Generates configuration for Visual Studio Code for given package(s) making it aware of the used dependencies.",
		.descriptionShort = "Generates configuration for Visual Studio Code",
		.arguments = {
			{
				.name = "packageName",
				.description = "Package name(s) to generate configuration for. Required, unless in a managed package directory in which case current package is assumed",
				.required = false,
				.list = true,
				.literal = false
			}
		}
	},
	{
		.command = "build",
		.alias = "",
		.description = "Compile package. Binaries are generated in /bin/Debug and /bin/Release. Release binaries are smaller and more performant, but do not include safeguards provided by debug binaries.",
		.descriptionShort = "Compile package",
		.arguments = {
			{
				.name = "packageName",
				.description = "Package name(s) to compile. Required, unless in a managed package directory in which case current package is assumed",
				.required = false,
				.list = true,
				.literal = false
			}
		}
	},
	{
		.command = "check",
		.alias = "",
		.description = "Check package(s) integrity. Non-managed packages only need to exist on the file system. Managed packages must contain structure:\n|-src\n|+includes\n|--src\n|--lib\n|-premake5.lua (only if strict check)",
		.descriptionShort = "Check package(s) integrity",
		.arguments = {
			{
				.name = "--all",
				.description = "Check all packages",
				.required = false,
				.list = false,
				.literal = true
			},
			{
				.name = "packageName",
				.description = "Package name(s) to check. Required, unless --all specified or within a managed package directory in which case current package is assumed.",
				.required = false,
				.list = true,
				.literal = false
			}
		}
	},
	{
		.command = "materialize",
		.alias = "",
		.description = "By default, cppm creates symbolic links in includes directory to link the dependencies to your package. This works for local compilation and development, but such package is not portable and will fail to compile if shared as such. When you run materialize, it copies all dependecy files into includes directory, producing a portable and distributable package, which should compile on any compatible system. You can revert this operation using unmaterialize command.",
		.descriptionShort = "Produce portable package by copying all dependecy files into includes directory",
		.arguments = {
			{
				.name = "packageName",
				.description = "Package name(s) to materialize. Required, unless within a managed package directory in which case current package is assumed.",
				.required = false,
				.list = true,
				.literal = false
			}
		}
	},
	{
		.command = "unmaterialize",
		.alias = "",
		.description = "Removes previously materialized dependency files from includes directory and re-establishes them as symbolic links. This produces a leaner and dynamic package, but it is not portable is such state. Use this if you previously used materialize to produce a distributable package, and want to resume development.",
		.descriptionShort = "Link dependencies using symbolic links",
		.arguments = {
			{
				.name = "packageName",
				.description = "Package name(s) to unmaterialize. Required, unless within a managed package directory in which case current package is assumed.",
				.required = false,
				.list = true,
				.literal = false
			}
		}
	},
	{
		.command = "push",
		.alias = "",
		.description = "Push distributable package to remote repository. Running this command is same as running cppm materialize, git push, cppm unmaterialize (in this exact sequence). This is needed because included packages are, by default, linked to you package using symbolic links, which would be pushed to remote as such, pointing to non-existent data when checked out on a remote machine. Materialize includes all dependency files in includes directory to produce a protable package which can be compiled when checked out on any compatible system. Once push to remote is done, includes are reverted to non-materialized (symbolic link) state. If you want to learn more, read help for materialize and unmaterialize commands.",
		.descriptionShort = "Push distributable package to remote repository",
		.arguments = {
			{
				.name = "packageName",
				.description = "Package name(s) to push to remote repository. Required, unless within a managed package directory in which case current package is assumed.",
				.required = false,
				.list = true,
				.literal = false
			}
		}
	},
	{
		.command = "help",
		.alias = "",
		.description = "List available commands, specify command to show details about it's usage.\n\nArgument syntax:\n? - argument is optional (although it may require you to be within a package dir)\n... - you can list multiple arguments at once\n[] - argument is a variable, not a literal",
		.descriptionShort = "List available commands, specify command to show details about it's usage",
		.arguments = {
			{
				.name = "command",
				.description = "If included show detailed description related to the command, otherwise list commands with a short description",
				.required = false,
				.list = false,
				.literal = false
			}
		}
	}
};

inline const char* HELP_GENERAL = "Usage cppm [command] [...options]";

namespace help {
	void printHelp(const char* command = nullptr);
	void printHelpItem(const HelpItem& item, bool shortDescription);
}