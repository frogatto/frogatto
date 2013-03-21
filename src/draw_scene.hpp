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
#ifndef DRAW_SCENE_HPP_INCLUDED
#define DRAW_SCENE_HPP_INCLUDED

#include <string>

#include "achievements.hpp"
#include "formula_callable.hpp"

namespace graphics {
class color;
class color_transform;
}

class entity;
class rect;
class level;

struct screen_position {
	screen_position() : init(false), x(0), y(0), focus_x(0), focus_y(0),
	                    flip_rotate(0), coins(-1),
						shake_x_offset(0),shake_y_offset(0),shake_x_vel(0),shake_y_vel(0), zoom(1), x_border(0), y_border(0)
	{}
	bool init;
	int x, y;
	int focus_x, focus_y;
	int	shake_x_offset,shake_y_offset;
	int shake_x_vel,shake_y_vel;
	int flip_rotate;
	int coins;
	float zoom;

	//area where the screen is too large for the level.
	int x_border, y_border;
};

//Measures the current draw position in centi-pixels.
screen_position& last_draw_position();

struct disable_flashes_scope {
	disable_flashes_scope();
	~disable_flashes_scope();
};

void screen_color_flash(const graphics::color_transform& color, const graphics::color_transform& color_delta, int duration);
void set_scene_title(const std::string& msg, int duration=150);
void set_displayed_achievement(achievement_ptr a);
bool is_achievement_displayed();


bool update_camera_position(const level& lvl, screen_position& pos, const entity* focus=NULL, bool do_draw=true);
void render_scene(const level& lvl, const screen_position& pos, const entity* focus=NULL, bool do_draw=true);

//draw_scene calls both update_camera_position() and then render_scene()
void draw_scene(const level& lvl, screen_position& pos, const entity* focus=NULL, bool do_draw=true);

struct performance_data : public game_logic::formula_callable {
	int fps;
	int cycles_per_second;
	int delay;
	int draw;
	int process;
	int flip;
	int cycle;
	int nevents;

	std::string profiling_info;

	performance_data(int fps_, int cycles_per_second_, int delay_, int draw_, int process_, int flip_, int cycle_, int nevents_, const std::string& profiling_info_)
	  : fps(fps_), cycles_per_second(cycles_per_second_), delay(delay_),
	    draw(draw_), process(process_), flip(flip_), cycle(cycle_),
		nevents(nevents_), profiling_info(profiling_info_)
	{}

	variant get_value(const std::string& key) const;
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const;

	static void set_current(const performance_data& d);
	static performance_data* current();
};

void draw_fps(const level& lvl, const performance_data& data);

void add_debug_rect(const rect& r);

#endif
