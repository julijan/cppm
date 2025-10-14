#include "JSONSerializer.h"
#include <iomanip>

JSONSerializer::JSONSerializer() {}

void JSONSerializer::stream(boost::json::value& json, std::ostream& stream) {
	stream << this->toString(json);
	stream.flush();
}

std::string JSONSerializer::toString(boost::json::value& json, size_t depth) {

	switch (json.kind()) {
		case boost::json::kind::object:
			this->object(json.as_object(), depth);
			break;
	
		case boost::json::kind::array:
			this->array(json.as_array());
			break;
	
		case boost::json::kind::int64:
		case boost::json::kind::uint64:
			this->number(json);
			break;
		
		case boost::json::kind::double_:
			this->decimal(json);
			break;
	
		case boost::json::kind::string:
			this->string(json.as_string());
			break;
	
		case boost::json::kind::bool_:
			this->bolean(json);
			break;
	
		case boost::json::kind::null:
			this->null();
			break;
	}

	return this->content.str();
}

std::string JSONSerializer::stringEscaped(std::string& val) {
	std::stringstream stream;

	for (char c: val) {
        switch (c) {
            case '"':  stream << "\\\""; break;
            case '\\': stream << "\\\\"; break;
            case '\b': stream << "\\b";  break;
            case '\f': stream << "\\f";  break;
            case '\n': stream << "\\n";  break;
            case '\r': stream << "\\r";  break;
            case '\t': stream << "\\t";  break;
            default:
                if (c < 0x20) {
                    // Control characters -> \u00XX
                    stream << "\\u"
                       << std::hex << std::uppercase << std::setw(4)
                       << std::setfill('0') << int(c)
                       << std::dec;
                } else {
                    stream << c;
                }
        }
	}

	return stream.str();
}

void JSONSerializer::clear() {
	this->content.clear();
}