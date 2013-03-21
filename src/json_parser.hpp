/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
bool file_exists_and_is_valid(const std::string& fname);

struct parse_error {
	explicit parse_error(const std::string& msg);
	parse_error(const std::string& msg, const std::string& filename, int line, int col);

	std::string error_message() const;

	std::string message;
	std::string fname;
	int line, col;
};

}

#endif
