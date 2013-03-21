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
#ifndef FORMATTER_HPP_INCLUDED
#define FORMATTER_HPP_INCLUDED

#include <sstream>

class formatter
{
public:
	template<typename T>
	formatter& operator<<(const T& o) {
		stream_ << o;
		return *this;
	}

	const std::string str() {
		return stream_.str();
	}

	const char* c_str() {
		return str().c_str();
	}

	operator std::string() {
		return stream_.str();
	}

private:
	std::ostringstream stream_;
};

#endif
