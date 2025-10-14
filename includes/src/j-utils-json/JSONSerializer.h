#pragma once

#include <cstddef>
#include <ostream>
#include <sstream>

#include "json/value.hpp"

// JSON serialization abstract class

class JSONSerializer {
public:
	JSONSerializer();

	void stream(boost::json::value& json, std::ostream& stream);
	std::string toString(boost::json::value& json, size_t depth = 0);

	void clear();

protected:

	std::stringstream content;

	virtual void object(boost::json::object& val, size_t depth = 0) = 0;
	virtual void array(boost::json::array& val) = 0;
	virtual void number(boost::json::value& val) = 0;
	virtual void decimal(boost::json::value& val) = 0;
	virtual void string(boost::json::string& val) = 0;
	virtual void bolean(boost::json::value& val) = 0;
	virtual void null() = 0;

	virtual std::string stringEscaped(std::string& val);

};