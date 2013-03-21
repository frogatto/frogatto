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
#ifndef LOAD_LEVEL_HPP_INCLUDED
#define LOAD_LEVEL_HPP_INCLUDED

#include <string>
#include <vector>

#include "variant.hpp"

class level;

struct load_level_manager {
	load_level_manager();
	~load_level_manager();
};

void load_level_paths();
void reload_level_paths();
const std::string& get_level_path(const std::string& name);

void clear_level_wml();
void preload_level_wml(const std::string& lvl);
variant load_level_wml(const std::string& lvl);
variant load_level_wml_nowait(const std::string& lvl);

void preload_level(const std::string& lvl);
level* load_level(const std::string& lvl);

std::vector<std::string> get_known_levels();

#endif
