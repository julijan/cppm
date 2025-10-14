#pragma once

#include "JSONSerializer.h"

// serializes JSON in a nice, human-readable way

class JSONSerializerNice: public JSONSerializer {
public:
	JSONSerializerNice();

private:
	void object(boost::json::object &val, size_t depth = 0) override;
	void array(boost::json::array &val) override;
	void number(boost::json::value &val) override;
	void decimal(boost::json::value &val) override;
	void string(boost::json::string &val) override;
	void bolean(boost::json::value &val) override;
	void null() override;
};