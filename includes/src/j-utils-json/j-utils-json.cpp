#include "j-utils-json.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "stream_parser.hpp"

namespace utils {
	namespace json {
		void write(const std::filesystem::path& file, const boost::json::value& json)
		{
			std::ofstream fh(file);
			if (!fh.is_open()) {
				throw std::runtime_error("Error opening file for writing");
			}
			fh << json;
	
			fh.flush();
			fh.close();
		}
	
		boost::json::value read(const std::filesystem::path &file)
		{
			if (!std::filesystem::exists(file)) {
				return boost::json::value();
			}
	
			boost::json::stream_parser parser;
			std::fstream stream(file);
	
			if (!stream.is_open()) {
				std::cerr << "Error opening JSON file for reading" << std::endl;
				return boost::json::array();
			}
	
			std::string line;
			while (std::getline(stream, line)) {
				parser.write(line);
			}
			parser.finish();
	
			return parser.release();
		}
	
		std::vector<std::string> fromJSONArray(const boost::json::array& arr)
		{
			std::vector<std::string> vec;
			for (const boost::json::value& item: arr) {
				vec.push_back(item.as_string().c_str());
			}
			return vec;
		}
	
		std::vector<std::string> extractArrayFromJSONObject(const boost::json::object& obj, const char* key)
		{
			if (!obj.contains(key)) {
				// key does not exist, return empty vector
				return std::vector<std::string>();
			}
	
			// key exists, assume it is a boost::json::array and return a vector from it
			return utils::json::fromJSONArray(obj.at(key).as_array());
		}
	
		boost::json::array toJSONArray(const std::vector<std::string>& arr)
		{
			boost::json::array arrJSON;
			for (const std::string& item: arr) {
				arrJSON.push_back(item.c_str());
			}
			return arrJSON;
		}
	}
}