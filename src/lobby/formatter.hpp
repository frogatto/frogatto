#pragma once
#ifndef FORMATTER_HPP_INCLUDED
#define FORMATTER_HPP_INCLUDED

#include <cstdint>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>

struct formatter
{
	template<typename T>
	formatter& operator<<(const T& o) {
		stream << o;
		return *this;
	}

	const std::string str() {
		return stream.str();
	}

	const char* c_str() {
		return str().c_str();
	}

	operator std::string() {
		return stream.str();
	}
	std::ostringstream stream;
};

template<> inline formatter& formatter::operator<<(const std::vector<uint8_t>& o) {
	for(auto c : o) {
		if(c < 32 || c > 127) {
			stream << "[" << std::setw(2) << std::setfill('0') << std::hex << int(c) << std::dec << "]";
		} else {
			stream << char(c);
		}
	}
	return *this;
}
#endif
