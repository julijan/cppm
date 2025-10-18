#include "j-utils-fs.h"
#include "j-utils-string.h"
#include <string>

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

		path relativePath(path from, path to) {
			std::string fromString = from.string();
			std::string toString = to.string();
			bool fromEndsWithSeparator = utils::string::endsWith(
				fromString,
				std::to_string(path::preferred_separator)
			);
			return to.string().substr(from.string().length() - (fromEndsWithSeparator ? 1 : 0));
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