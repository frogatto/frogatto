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
#ifndef OBJECT_EVENTS_HPP_INCLUDED
#define OBJECT_EVENTS_HPP_INCLUDED

enum OBJECT_EVENT_ID {
	OBJECT_EVENT_ANY,
	OBJECT_EVENT_START_LEVEL,
	OBJECT_EVENT_PLAYER_DEATH,
	OBJECT_EVENT_LOAD,
	OBJECT_EVENT_LOAD_CHECKPOINT,
	OBJECT_EVENT_CREATE,
	OBJECT_EVENT_DONE_CREATE,
	OBJECT_EVENT_BECOME_ACTIVE,
	OBJECT_EVENT_SURFACE_DAMAGE,
	OBJECT_EVENT_ENTER_ANIM,
	OBJECT_EVENT_END_ANIM,
	OBJECT_EVENT_COLLIDE_LEVEL,
	OBJECT_EVENT_COLLIDE_HEAD,
	OBJECT_EVENT_COLLIDE_FEET,
	OBJECT_EVENT_COLLIDE_DAMAGE,
	OBJECT_EVENT_COLLIDE_SIDE,
	OBJECT_EVENT_STUCK,
	OBJECT_EVENT_JUMPED_ON,
	OBJECT_EVENT_GET_HIT,
	OBJECT_EVENT_PROCESS,
	OBJECT_EVENT_TIMER,
	OBJECT_EVENT_ENTER_WATER,
	OBJECT_EVENT_EXIT_WATER,
	OBJECT_EVENT_CHANGE_SOLID_DIMENSIONS_FAIL,
	OBJECT_EVENT_ADD_OBJECT_FAIL,
	OBJECT_EVENT_CHANGE_ANIMATION_FAILURE,
	OBJECT_EVENT_DIE,
	OBJECT_EVENT_INTERACT,
	OBJECT_EVENT_CHILD_SPAWNED,
	OBJECT_EVENT_SPAWNED,
	OBJECT_EVENT_DRAW,
	OBJECT_EVENT_BEGIN_DIALOG,
	OBJECT_EVENT_COSMIC_SHIFT,
	OBJECT_EVENT_SCHEDULE_FINISHED,
	OBJECT_EVENT_OUTSIDE_LEVEL,
	OBJECT_EVENT_BEING_ADDED,
	OBJECT_EVENT_BEING_REMOVED,
	NUM_OBJECT_BUILTIN_EVENT_IDS,
};

const std::string& get_object_event_str(int id);
int get_object_event_id(const std::string& str);

#endif
