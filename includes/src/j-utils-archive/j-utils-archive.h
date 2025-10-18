#pragma once

#include <filesystem>

namespace utils {

	namespace archive {
	
		void compress(std::filesystem::path inputPath, std::filesystem::path outFilePath);

	}

}