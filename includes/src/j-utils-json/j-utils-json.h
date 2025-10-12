#include <filesystem>
#include <vector>
#include "array.hpp"

namespace utils {
	namespace json {
		void write(const std::filesystem::path& file, const boost::json::value& json);
		boost::json::value read(const std::filesystem::path& file);
	
		// produce a boost::json::array from given vector
		boost::json::array toJSONArray(const std::vector<std::string>& arr);
	
		// produce a vector from given boost::json::array
		std::vector<std::string> fromJSONArray(const boost::json::array& arr);
	
		// safely extract array from given JSON object
		// if key exists in given boost::json::object, returns vector from it
		// if key does not exist, returns an empty vector
		std::vector<std::string> extractArrayFromJSONObject(const boost::json::object& obj, const char* key);
	}
}