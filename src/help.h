#pragma once

#include <vector>

struct HelpArguments {
	const char* name;
	const char* description;
	bool required;
	bool list;
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
				.list = false
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
		.description = "Show package details, which include:\nPackage name: string\nVersion: string\nType: PackageType\nPath: string\nDependencies: list\n\nProjectType: ConsoleApp | WindowedApp | StaticLib | SharedLib",
		.descriptionShort = "Show package details",
		.arguments = {
			{
				.name = "packageName",
				.description = "Required unless command is executed in a package directory, in which case description for current package is shown",
				.required = false,
				.list = false
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
				.list = true
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
				.list = true
			}
		}
	},
	{
		.command = "register",
		.alias = "",
		.description = "Register a 3rd party code as a non-managed package. This allows you to easily include it in your managed packages using include. To use this, cd to 3rd party package directory and run cppm register. You will be able to assign a name to package when you run the command, assumed name is the directory name. Good example for using this is if you use vcpkg or other package managers, you can register the installed packages, and include them with a single command. See cppm help include for details on how to include packages.",
		.descriptionShort = "Register a 3rd party code as a package",
		.arguments = {}
	},
	{
		.command = "unregister",
		.alias = "",
		.description = "Remove previously registered non-managed package from the registry. If none of your packages depend on it, it will simply remove it from the registry, otherwise you will be prompted whether you want to proceed.",
		.descriptionShort = "Remove previously registered non-managed package from the registry",
		.arguments = {
			{
				.name = "packageName",
				.description = "Package name to unregister. Required, unless you are within the package directory in which case, if ommitted, current package is assumed",
				.required = false,
				.list = true
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
				.list = true
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
				.list = true
			}
		}
	},
	{
		.command = "help",
		.alias = "",
		.description = "List available commands, specify command to show details about it's usage.\n\nArgument syntax:\n? - argument is optional (although it may require you to be within a package dir)\n... - you can list multiple arguments at once",
		.descriptionShort = "List available commands, specify command to show details about it's usage",
		.arguments = {
			{
				.name = "command",
				.description = "If included show detailed description related to the command, otherwise list commands with a short description",
				.required = false,
				.list = false
			}
		}
	}
};

inline const char* HELP_GENERAL = "Usage cppm [command] [...options]";