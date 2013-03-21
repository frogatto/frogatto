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
#ifndef FROGATTO_FUNCTIONAL_HPP_INCLUDED
#define FROGATTO_FUNCTIONAL_HPP_INCLUDED


#include <vector>

template<typename To, typename From, typename Fn>
std::vector<To> map_vector(const std::vector<From>& v, Fn fn) {
	std::vector<To> result;
	result.reserve(v.size());
	for(typename std::vector<From>::const_iterator i = v.begin(); i != v.end(); ++i) {
		result.push_back(fn(*i));
	}

	return result;
}

#endif
