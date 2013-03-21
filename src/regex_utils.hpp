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
#ifndef REGEX_UTILS_HPP_INCLUDED
#define REGEX_UTILS_HPP_INCLUDED

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

template<typename T1>
bool match_regex(const std::string& str, const boost::regex& pattern, T1* out1)
{
	boost::cmatch match;
	const bool result = boost::regex_match(str.c_str(), match, pattern);
	if(!result) {
		return false;
	}

	std::string s1(match[1].first, match[1].second);
	try {
		*out1 = boost::lexical_cast<T1>(s1);
	} catch(boost::bad_lexical_cast&) {
		return false;
	}

	return true;
}

template<typename T1, typename T2>
bool match_regex(const std::string& str, const boost::regex& pattern, T1* out1, T2* out2)
{
	boost::cmatch match;
	const bool result = boost::regex_match(str.c_str(), match, pattern);
	if(!result) {
		return false;
	}

	std::string s1(match[1].first, match[1].second);
	try {
		*out1 = boost::lexical_cast<T1>(s1);
	} catch(boost::bad_lexical_cast&) {
		return false;
	}

	std::string s2(match[2].first, match[2].second);
	try {
		*out2 = boost::lexical_cast<T2>(s2);
	} catch(boost::bad_lexical_cast&) {
		return false;
	}

	return true;
}

template<typename T1, typename T2, typename T3>
bool match_regex(const std::string& str, const boost::regex& pattern, T1* out1, T2* out2, T3* out3)
{
	boost::cmatch match;
	const bool result = boost::regex_match(str.c_str(), match, pattern);
	if(!result) {
		return false;
	}

	std::string s1(match[1].first, match[1].second);
	try {
		*out1 = boost::lexical_cast<T1>(s1);
	} catch(boost::bad_lexical_cast&) {
		return false;
	}

	std::string s2(match[2].first, match[2].second);
	try {
		*out2 = boost::lexical_cast<T2>(s2);
	} catch(boost::bad_lexical_cast&) {
		return false;
	}

	std::string s3(match[3].first, match[3].second);
	try {
		*out3 = boost::lexical_cast<T1>(s3);
	} catch(boost::bad_lexical_cast&) {
		return false;
	}

	return true;
}

#endif
