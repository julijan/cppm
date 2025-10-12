#include "core.h"
#include "package.h"
#include "utils.h"
#include "j-utils-system/j-utils-system.h"

std::filesystem::path Core::path() {
	return utils::system::appDataDir() + std::filesystem::path::preferred_separator + ".cppm";
}

void Core::initDirectory()
{
	if (!Core::directoryExists()) {
		utils::fs::mkdir(Core::path());
		Package::initRegistry();
	}
}

bool Core::directoryExists()
{
	return utils::fs::pathExists(Core::path());
}

std::filesystem::path Core::filePath(const char* const fileName) {
	return std::filesystem::path(Core::path()).append(fileName);
}