#pragma once

#include <string>
#include <functional>
#include <algorithm>

namespace utils {

	namespace vector {
		// join vector elements using provided glue
		std::string join(const std::vector<std::string>& vec, const std::string& glue);

		// transform each element running it through a predicate
		// retuning a new vector of transformed elements
		template <typename R, typename T>
		std::vector<R> map(const std::vector<T>& vec, std::function<R(const T&)> pred)
		{
			std::vector<std::string> res;
			std::transform(vec.begin(), vec.end(), std::back_inserter(res), pred);
			return res;
		}

		// returns a string: "el0", "el1", "el2", ...
		std::string toQuotedList(const std::vector<std::string>& vec);
	}

	namespace time {
		int unixTimestamp();
	}
}