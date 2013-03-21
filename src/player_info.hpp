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
#ifndef PLAYER_INFO_HPP_INCLUDED
#define PLAYER_INFO_HPP_INCLUDED

#include <map>
#include <string>
#include <vector>

#include "variant.hpp"

//class which contains information about the player.
class player_info
{
public:
	explicit player_info(entity& e) : entity_(&e), slot_(0)
	{}
	player_info(entity& e, variant node);
	
	void object_destroyed(const std::string& level_id, int item);
	const std::vector<int>& get_objects_destroyed(const std::string& level_id) const;

	variant write() const;

	void swap_player_state(player_info& player) {
		items_destroyed_.swap(player.items_destroyed_);
		objects_destroyed_.swap(player.objects_destroyed_);
	}

	const entity& get_entity() const { return *entity_; }
	entity& get_entity() { return *entity_; }

	void set_entity(entity& e) { entity_ = &e; }

	const std::string& current_level() const { return current_level_; }
	void set_current_level(const std::string& lvl) { current_level_ = lvl; }

	void set_player_slot(int slot) { slot_ = slot; }

	void read_controls(int cycle);

	
	bool reverse_global_vertical_zordering() const { return entity_->reverse_global_vertical_zordering(); }

private:
	entity* entity_;

	mutable std::map<std::string, std::vector<int> > items_destroyed_;
	mutable std::map<std::string, std::vector<int> > objects_destroyed_;
	
	//the number of the player.
	int slot_;

	std::string current_level_;
};

#endif
