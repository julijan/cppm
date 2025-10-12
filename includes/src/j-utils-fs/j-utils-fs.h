#include <filesystem>
#include <functional>

template <typename T, unsigned int S>
using SmartArray = std::array<T, S>;

namespace utils {
	
	namespace fs {
		using namespace std::filesystem;

		bool pathExists(path p);

		void mkdir(path p);

		path currentPath();

		// append all given sub-directories to given path
		// returns a new path
		template <int Depth>
		inline std::filesystem::path extendPath(const std::filesystem::path& p, SmartArray<const char*, Depth> subdirs) {
			std::filesystem::path path(p);

			for (const char* subdir: subdirs) {
				path.append(subdir);
			}

			return path;
		};

		// return a vector of all paths for which the predicate returns true
		// predicate will receive all paths recursively, starting from the entry
		std::vector<path> filterRecursive(path entry, std::function<bool(const path& path)> predicate);
	}
}