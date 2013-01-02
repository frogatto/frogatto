
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "string_utils.hpp"
#include "unit_test.hpp"

#include <algorithm>
#include <stdio.h>

#include "compat.hpp"

namespace util
{

bool c_isalnum(int c)
{
	return ::isalnum(static_cast<unsigned char>(c));
}

bool c_isalpha(int c)
{
	return ::isalpha(static_cast<unsigned char>(c));
}

bool c_isascii(int c)
{
	return isascii(static_cast<unsigned char>(c));
}

bool c_isblank(int c)
{
#if defined(_WINDOWS)
	return __isblank(c);
#else
	return ::isblank(static_cast<unsigned char>(c));
#endif
}

bool c_iscntrl(int c)
{
	return ::iscntrl(static_cast<unsigned char>(c));
}

bool c_isdigit(int c)
{
	return ::isdigit(static_cast<unsigned char>(c));
}

bool c_isgraph(int c)
{
	return ::isgraph(static_cast<unsigned char>(c));
}

bool c_islower(int c)
{
	return ::islower(static_cast<unsigned char>(c));
}

bool c_isprint(int c)
{
	return ::isprint(static_cast<unsigned char>(c));
}

bool c_ispunct(int c)
{
	return ::ispunct(static_cast<unsigned char>(c));
}

bool c_isspace(int c)
{
	return ::isspace(static_cast<unsigned char>(c));
}

bool c_isupper(int c)
{
	return ::isupper(static_cast<unsigned char>(c));
}

bool c_isxdigit(int c)
{
	return ::isxdigit(static_cast<unsigned char>(c));
}

bool c_isnewline(char c)
{
	return c == '\r' || c == '\n';
}

bool portable_isspace(char c)
{
	return c_isnewline(c) || c_isspace(c);
}

bool notspace(char c)
{
	return !portable_isspace(c);
}

std::string &strip(std::string &str)
{
	std::string::iterator it = std::find_if(str.begin(), str.end(), notspace);
	str.erase(str.begin(), it);
	str.erase(std::find_if(str.rbegin(), str.rend(), notspace).base(), str.end());

	return str;
}

std::vector<std::string> split(std::string const &val, const std::string& delim)
{
	std::vector<std::string> result;
	if(delim.empty()) {
		foreach(char c, val) {
			result.push_back(std::string(1, c));
		}

		return result;
	}

	const char* ptr = val.c_str();
	for(;;) {
		const char* end = strstr(ptr, delim.c_str());
		if(end == NULL) {
			result.push_back(std::string(ptr));
			return result;
		}

		result.push_back(std::string(ptr, end));
		ptr = end + delim.size();
	}

	return result;
}

std::vector<std::string> split(std::string const &val, char c, int flags)
{
	std::vector<std::string> res;
	split(val, res, c, flags);
	return res;
}

void split(std::string const &val, std::vector<std::string>& res, char c, int flags)
{
	std::string::const_iterator i1 = val.begin();
	std::string::const_iterator i2 = val.begin();

	while (i2 != val.end()) {
		if (*i2 == c) {
			std::string new_val(i1, i2);
			if (flags & STRIP_SPACES)
				strip(new_val);
			if (!(flags & REMOVE_EMPTY) || !new_val.empty())
				res.push_back(new_val);
			++i2;
			if (flags & STRIP_SPACES) {
				while (i2 != val.end() && *i2 == ' ')
					++i2;
			}

			i1 = i2;
		} else {
			++i2;
		}
	}

	std::string new_val(i1, i2);
	if (flags & STRIP_SPACES)
		strip(new_val);
	if (!(flags & REMOVE_EMPTY) || !new_val.empty())
		res.push_back(new_val);
}

std::string join(const std::vector<std::string>& v, char j)
{
	std::string res;
	for(int n = 0; n != v.size(); ++n) {
		if(n != 0) {
			res.push_back(j);
		}

		res += v[n];
	}

	return res;
}

const char* split_into_ints(const char* s, int* output, int* output_size)
{
	char* endptr = NULL;
	int index = 0;
	for(;;) {
		int result = strtol(s, &endptr, 10);
		if(endptr == s) {
			break;
		}

		if(index < *output_size) {
			output[index] = result;
		}

		++index;

		if(*endptr != ',') {
			break;
		}

		s = endptr+1;
	}

	*output_size = index;
	return endptr;
}

std::vector<int> split_into_vector_int(const std::string& s)
{
	std::vector<std::string> v = util::split(s);
	std::vector<int> result(v.size());
	for(int n = 0; n != v.size(); ++n) {
		result[n] = atoi(v[n].c_str());
	}

	return result;
}

std::string join_ints(const int* ints, int size)
{
	std::string result;
	char buf[256];
	for(int n = 0; n != size; ++n) {
		if(n != 0) {
			result += ",";
		}

		sprintf(buf, "%d", ints[n]);
		result += buf;
	}

	return result;
}

bool string_starts_with(const std::string& target, const std::string& prefix) {
	if(target.length() < prefix.length()) {
		return false;
	}
	std::string target_pfx =  target.substr(0,prefix.length());
	return target_pfx == prefix;
}

std::string strip_string_prefix(const std::string& target, const std::string& prefix) {
	if(target.length() < prefix.length()) {
		return "";
	}
	return target.substr(prefix.length());
}

}

UNIT_TEST(test_split_into_ints)
{
	int buf[6];
	int buf_size = 6;
	const char* str = "4,18,7,245";
	const char* res = util::split_into_ints(str, buf, &buf_size);
	CHECK_EQ(buf_size, 4);
	CHECK_EQ(res, str + strlen(str));
	CHECK_EQ(buf[0], 4);
	CHECK_EQ(buf[1], 18);
	CHECK_EQ(buf[2], 7);
	CHECK_EQ(buf[3], 245);

	buf[1] = 0;
	buf_size = 1;
	res = util::split_into_ints(str, buf, &buf_size);
	CHECK_EQ(buf_size, 4);
	CHECK_EQ(res, str + strlen(str));
	CHECK_EQ(buf[0], 4);
	CHECK_EQ(buf[1], 0);
}
