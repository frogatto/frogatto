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
#ifndef CONTROLS_HPP_INCLUDED
#define CONTROLS_HPP_INCLUDED

#include <boost/scoped_ptr.hpp>

#include <vector>
#include <cstddef>

#include "graphics.hpp"

#if SDL_VERSION_ATLEAST(2, 0, 0)
typedef SDL_Keycode key_type;
#else
typedef SDLKey key_type;
#endif

namespace controls {

enum CONTROL_ITEM {
	CONTROL_UP,
	CONTROL_DOWN,
	CONTROL_LEFT,
	CONTROL_RIGHT,
	CONTROL_ATTACK,
	CONTROL_JUMP,
	CONTROL_TONGUE,
	NUM_CONTROLS,
};

void set_keycode(CONTROL_ITEM item, key_type key);
key_type get_keycode(CONTROL_ITEM item);

void new_level(int starting_cycle, int nplayers, int local_player);

//an object which can lock controls into a specific state for the duration
//of its scope.
class local_controls_lock {
public:
	explicit local_controls_lock(unsigned char state=0);
	~local_controls_lock();
};

class control_backup_scope_impl;
class control_backup_scope {
public:
	control_backup_scope();
	~control_backup_scope();
private:
	boost::scoped_ptr<control_backup_scope_impl> impl_;
};

void read_until(int ncycle);
int local_controls_end();
void read_local_controls();
void unread_local_controls();
void ignore_current_keypresses();

void get_control_status(int cycle, int player, bool* output);
void set_delay(int delay);

void read_control_packet(const char* buf, size_t len);
void write_control_packet(std::vector<char>& v);

int first_invalid_cycle();
void mark_valid();

int num_players();
int num_errors();
int packets_received();
int cycles_behind();

int their_highest_confirmed();
int last_packet_size();

void set_checksum(int cycle, int sum);

void debug_dump_controls();

}

#endif
