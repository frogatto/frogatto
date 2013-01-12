
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
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
