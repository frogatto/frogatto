#ifndef JSON_PARSER_HPP_INCLUDED
#define JSON_PARSER_HPP_INCLUDED

#include <string>

#include "variant.hpp"

namespace json {
void set_file_contents(const std::string& path, const std::string& contents);
std::string get_file_contents(const std::string& path);

enum JSON_PARSE_OPTIONS { JSON_NO_PREPROCESSOR = 0, JSON_USE_PREPROCESSOR };
variant parse(const std::string& doc, JSON_PARSE_OPTIONS options=JSON_USE_PREPROCESSOR);
variant parse_from_file(const std::string& fname, JSON_PARSE_OPTIONS options=JSON_USE_PREPROCESSOR);

struct parse_error {
	explicit parse_error(const std::string& msg);
	parse_error(const std::string& msg, int line, int col);

	std::string error_message() const;

	std::string message;
	std::string fname;
	int line, col;
};

}

#endif
