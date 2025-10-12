#include "j-utils-fs.h"

namespace utils {

	namespace fs {
		using namespace std::filesystem;

		bool pathExists(path p) {
			return exists(p);
		}

		void mkdir(path p)
		{
			create_directory(p);
		}

		path currentPath()
		{
			return current_path();
		}

		std::vector<path> filterRecursive(path entry, std::function<bool(const path& path)> predicate) {
			std::vector<path> results;

			if (predicate(entry)) {
				// entry itself matches predicate
				results.push_back(entry);
			}

			// scan entry
			directory_iterator iter(entry);
			for (auto entry: iter) {
				path entryPath = entry.path();

				// path in entry matches predicate
				if (predicate(entryPath)) {
					results.push_back(entryPath);
				}

				if (is_directory(entryPath)) {
					// descend recursively
					std::vector<path> resultsNext = filterRecursive(entryPath, predicate);
					results.insert(results.begin(), resultsNext.begin(), resultsNext.end());
				}
			}

			return results;
		}
	}
}