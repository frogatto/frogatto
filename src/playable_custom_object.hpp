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
#ifndef PLAYABLE_CUSTOM_OBJECT_HPP_INCLUDED
#define PLAYABLE_CUSTOM_OBJECT_HPP_INCLUDED

#include <boost/scoped_ptr.hpp>

#include "controls.hpp"
#include "custom_object.hpp"
#include "player_info.hpp"
#include "variant.hpp"

class level;

class playable_custom_object : public custom_object
{
public:
	playable_custom_object(const custom_object& obj);
	playable_custom_object(const playable_custom_object& obj);
	playable_custom_object(variant node);

	virtual variant write() const;

	virtual player_info* is_human() { return &player_info_; }
	virtual const player_info* is_human() const { return &player_info_; }

	void save_game();
	entity_ptr save_condition() const { return save_condition_; }

	virtual entity_ptr backup() const;
	virtual entity_ptr clone() const;

	virtual int vertical_look() const { return vertical_look_; }

	virtual bool is_active(const rect& screen_area) const;

	bool can_interact() const { return can_interact_ != 0; }

	int difficulty() const { return difficulty_; }

private:
	bool on_platform() const;

	int walk_up_or_down_stairs() const;

	virtual void process(level& lvl);
	variant get_value(const std::string& key) const;	
	void set_value(const std::string& key, const variant& value);

	player_info player_info_;

	int difficulty_;

	entity_ptr save_condition_;

	int vertical_look_;

	int underwater_ctrl_x_, underwater_ctrl_y_;

	bool underwater_controls_;

	int can_interact_;
	
	bool reverse_ab_;

	boost::scoped_ptr<controls::local_controls_lock> control_lock_;

	void operator=(const playable_custom_object);
};

#endif
