#include <cstring>
#include <string>
#include <iostream>

#include "help.h"
#include "utils.h"

namespace help {

	void printHelp(const char *command)
	{
		if (command == nullptr) {
			// general help, list commands with short description
			std::cout << HELP_GENERAL << '\n' << std::endl;
			std::cout << "Commands:" << std::endl;
			for (auto command: HELP_ITEMS) {
				printHelpItem(command, true);
			}
		} else {
			auto it = std::find_if(HELP_ITEMS.begin(), HELP_ITEMS.end(), [command](const HelpItem& item) {
				return strcmp(item.command, command) == 0 || strcmp(item.alias, command) == 0;
			});
			if (it == HELP_ITEMS.end()) {
				// requested help for a non-recognized command, show general help
				std::cerr << "Command " << command << " not recognized\n" << std::endl;
				return printHelp();
			}
	
			printHelpItem(*it, false);		
		}
	}
	
	void printHelpItem(const HelpItem &item, bool shortDescription)
	{
		std::string arglist = "";
		for (const HelpArguments& arg: item.arguments) {
			arglist += " ";
			arglist += help::argumentString(arg);
		}
		if (shortDescription) {
			std::cout << "  " << item.command << arglist << " - " << item.descriptionShort << std::endl;
		} else {
			std::cout << "Usage: cppm " << item.command << arglist << std::endl;
	
			if (strlen(item.alias) > 0) {
				std::cout << "Alias: " << item.alias << std::endl;
			}
	
			std::cout << std::endl;
	
			std::cout << "Description:" << std::endl;
			std::cout << utils::string::toWidth(item.description, 100, false) << std::endl;
			std::cout << std::endl;
		}
		if (!shortDescription) {
			if (item.arguments.size() == 0) {
				std::cout << "Command requires no arguments" << std::endl;
			} else {
				std::cout << "Arguments:" << std::endl;
				for (const HelpArguments& arg: item.arguments) {
					std::cout << "  " << help::argumentString(arg) << " - " << arg.description << std::endl;
				}
			}
		}
	}

	std::string argumentString(const HelpArguments& arg) {
		std::string argstring = "";
		argstring += (arg.literal ? "" : "[");
		argstring += (arg.required ? "" : "?");
		argstring += (arg.list ? "..." : "");
		argstring += arg.name;
		argstring += (arg.literal ? "" : "]");
		return argstring;
	}
}
