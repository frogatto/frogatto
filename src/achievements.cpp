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
#include <algorithm>
#include <map>

#include "achievements.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formula_callable.hpp"
#include "i18n.hpp"
#include "json_parser.hpp"
#include "preferences.hpp"
#include "string_utils.hpp"
#include "of_bridge.h"
#include "variant.hpp"

namespace {
std::map<std::string, achievement_ptr> cache;
}

achievement_ptr achievement::get(const std::string& id)
{
	if(cache.empty()) {
		variant node;
		try {
			node = (json::parse_from_file("data/achievements.cfg"));
		} catch(json::parse_error&) {
			cache[""]; //mark as loaded
			return achievement_ptr();
		}

		
		foreach(variant achievement_node, node["achievement"].as_list()) {
			achievement_ptr a(new achievement(achievement_node));
			cache[a->id()] = a;
		}
	}

	return cache[id];
}

achievement::achievement(variant node)
  : id_(node["id"].as_string()), name_(i18n::tr(node["name"].as_string())),
    description_(i18n::tr(node["description"].as_string())),
	points_(node["points"].as_int()),
	of_id_(node["of_id"].as_int())
{
}

namespace {
std::vector<std::string>* achievements = NULL;
}

bool attain_achievement(const std::string& id)
{
	if(achievements == NULL) {
		achievements = new std::vector<std::string>;
		variant val = preferences::registry()->query_value("achievements");
		if(val.is_string()) {
			*achievements = util::split(val.as_string());
			std::sort(achievements->begin(), achievements->end());
		}
	}

	if(std::binary_search(achievements->begin(), achievements->end(), id)) {
		return false;
	}
	
	#ifdef ENABLE_OPENFEINT
	achievement_ptr a = achievement::get(id);
	of_earn_achievement(a->of_id());
	#endif

	achievements->push_back(id);
	std::sort(achievements->begin(), achievements->end());

	preferences::registry()->mutate_value("achievements", variant(util::join(*achievements)));

	return true;
}
