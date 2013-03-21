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
#ifndef STATS_HPP_INCLUDED
#define STATS_HPP_INCLUDED

#include "variant.hpp"

void init_tables(const variant& doc);
void init_tables_for_module(const std::string& module, const variant& doc);
variant get_tables_definition();

std::map<std::string, std::string> get_stats_errors();

void read_stats(const variant& doc);
variant write_stats();

void process_stats(const variant& doc);

variant get_stats(const std::string& version, const std::string& module, const std::string& module_version, const std::string& lvl);

#endif
