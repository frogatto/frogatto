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

#include <boost/shared_ptr.hpp>

#include "geometry.hpp"
#include "thread.hpp"
#include "variant.hpp"

#include <string>

void http_upload(const std::string& payload, const std::string& script,
                 const char* hostname=NULL, const char* port=NULL);

namespace stats {

//download stats for a given level.
bool download(const std::string& lvl);

class manager {
public:
	manager();
	~manager();
};

class entry {
public:
	explicit entry(const std::string& type);
	entry(const std::string& type, const std::string& level_id);
	~entry();
	entry& set(const std::string& name, const variant& value);
	entry& add_player_pos();
private:
	entry(const entry& o);
	void operator=(const entry& o) const;

	std::string level_id_;
	std::map<variant, variant> records_;
};

void record_program_args(const std::vector<std::string>& args);

void record(const variant& value);
void record(const variant& value, const std::string& level_id);

void flush();
void flush_and_quit();

}

#endif
