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
#include <boost/bimap.hpp>

#include "asserts.hpp"
#include "difficulty.hpp"
#include "json_parser.hpp"
#include "module.hpp"
#include "variant.hpp"

namespace difficulty {

namespace {

typedef boost::bimap<std::string,int> diffculty_map_type;
typedef diffculty_map_type::value_type position;

diffculty_map_type& get_difficulty_map()
{
	static diffculty_map_type res;
	return res;
}

void create_difficulty_map()
{
	variant diff = json::parse_from_file("data/difficulty.cfg");
	// Any option is always defined.
	get_difficulty_map().insert(position("any", -1));
	for(int i = 0; i < diff["difficulties"].num_elements(); i++) {
		get_difficulty_map().insert(position(diff["difficulties"][i]["text"].as_string(), diff["difficulties"][i]["value"].as_int()));
	}
}

}

manager::manager()
{
	create_difficulty_map();
}

manager::~manager()
{
}

std::string to_string(int diff)
{
	diffculty_map_type::right_const_iterator it = get_difficulty_map().right.find(diff);
	if(it == get_difficulty_map().right.end()) {
		std::cerr << "Unrecognised difficulty value: \"" << diff << "\", please see the file data/difficulties.cfg for a list" << std::endl;
		return "";
	}
	return it->second;
}

int from_string(const std::string& s)
{
	diffculty_map_type::left_const_iterator it = get_difficulty_map().left.find(s);
	if(it == get_difficulty_map().left.end()) {
		//std::cerr << "Unrecognised difficulty value: \"" << s << "\", please see the file data/difficulties.cfg for a list" << std::endl;
		//return -1;
		ASSERT_LOG(false, "Unrecognised difficulty value: \"" << s << "\", please see the file data/difficulties.cfg for a list");
		return -1;
	}
	return it->second;
}

int from_variant(variant node)
{
	if(node.is_string()) {
		return from_string(node.as_string());
	}
	return node.as_int(-1);
}

}
