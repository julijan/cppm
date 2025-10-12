#include <functional>
#include <string>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <vector>

#include "utils.h"

namespace utils {

	namespace time {
		int unixTimestamp() {
			auto now = std::chrono::system_clock::now();
			auto sinceEpoch = now.time_since_epoch();
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch);
			return seconds.count();
		}
	}

}

std::string utils::vector::join(const std::vector<std::string>& vec, const std::string& glue)
{
	std::string joined = "";
	for (auto i = vec.cbegin(); i != vec.cend(); ++i) {
		joined += *i;
		if (i != vec.cend() - 1) {
			joined += glue;
		}
	}
	return joined;
}

std::string utils::vector::toQuotedList(const std::vector<std::string>& vec)
{
	return utils::vector::join(
		utils::vector::map<std::string, std::string>(vec, [](const std::string& item) {
			std::string out = "";
			out += '"' + item + '"';
			return out;
		}),
		", "
	);
}
