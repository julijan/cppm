#include <iomanip>

#include "JSONSerializerNice.h"
#include "j-utils-string.h"

JSONSerializerNice::JSONSerializerNice() {}

void JSONSerializerNice::object(boost::json::object& val, size_t depth) {
	this->content << utils::string::repeat("\t", depth) << "{\n";

	bool first = true;
	for (auto pair: val) {
		if (!first) {
			this->content << ",\n";
		}
		this->content << utils::string::repeat("\t", depth + 1) << "\"" << pair.key() << "\": ";
		this->toString(pair.value(), depth + 1);
		first = false;
	}

	this->content << "\n" << utils::string::repeat("\t", depth) << "}";
}

void JSONSerializerNice::array(boost::json::array &arr) {
	this->content << "[\n";
	bool first = true;
	for (auto val: arr) {
		if (!first) {
			this->content << ", ";
		}
		this->toString(val);
		first = false;
	}
	this->content << "\n]";
}

void JSONSerializerNice::number(boost::json::value &val) {
	this->content << val.get_int64();
}

void JSONSerializerNice::decimal(boost::json::value &val) {
	this->content << std::fixed << std::setprecision(6) << val.get_double();
}

void JSONSerializerNice::bolean(boost::json::value &val) {
	this->content << (val.get_bool() ? "true" : "false");
}

void JSONSerializerNice::null() {
	this->content << "null";
}

void JSONSerializerNice::string(boost::json::string &val) {
	std::string str = std::string(val.c_str());
	this->content << '"' << this->stringEscaped(str) << '"';
}
