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
#include <iostream>
#include <map>

#include "translate.hpp"

namespace i18n {

namespace {
typedef std::map<std::string,std::string> translation_map;
translation_map map;
}

void add_translation(const std::string& from, const std::string& to)
{
	std::cerr << "add translation: " << from << " -> " << to << "\n";
	map[from] = to;
}

const std::string& translate(const std::string& from)
{
	const translation_map::const_iterator i = map.find(from);
	if(i != map.end()) {
		return i->second;
	} else {
		return from;
	}
}

}
