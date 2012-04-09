
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef STRING_UTILS_HPP_INCLUDED
#define STRING_UTILS_HPP_INCLUDED

#include "foreach.hpp"

#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

namespace util
{

bool c_isalnum(int c);
bool c_isalpha(int c);
bool c_isascii(int c); //
bool c_isblank(int c); //
bool c_iscntrl(int c); //
bool c_isdigit(int c);
bool c_isgraph(int c); //
bool c_islower(int c);
bool c_isprint(int c);
bool c_ispunct(int c); //
bool c_isspace(int c);
bool c_isupper(int c); //
bool c_isxdigit(int c);

bool c_isnewline(char c);
bool portable_isspace(char c);
bool notspace(char c);

std::string& strip(std::string& str);

enum { REMOVE_EMPTY = 0x01, STRIP_SPACES = 0x02 };
std::vector<std::string> split(std::string const &val, char c = ',', int flags = REMOVE_EMPTY | STRIP_SPACES);
void split(std::string const &val, std::vector<std::string>& res, char c = ',', int flags = REMOVE_EMPTY | STRIP_SPACES);
std::vector<std::string> split(std::string const &val, std::string const &delim);
std::string join(const std::vector<std::string>& v, char c=',');

//splits the string 's' into ints, storing the output in 'output'. s
//should point to a comma-separated list of integers. output_size should point
//to the size of 'output'. The number of ints found will be stored in
//output_size.
const char* split_into_ints(const char* s, int* output, int* output_size);
std::vector<int> split_into_vector_int(const std::string& s);

std::string join_ints(const int* buf, int size);

bool string_starts_with(const std::string& target, const std::string& prefix);
std::string strip_string_prefix(const std::string& target, const std::string& prefix);

template<typename To, typename From>
std::vector<To> vector_lexical_cast(const std::vector<From>& v) {
	std::vector<To> result;
	result.resize(v.size());
	foreach(const From& from, v) {
		result.push_back(boost::lexical_cast<To>(from));
	}

	return result;
}

}

#endif
