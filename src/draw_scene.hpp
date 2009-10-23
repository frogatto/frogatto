#ifndef DRAW_SCENE_HPP_INCLUDED
#define DRAW_SCENE_HPP_INCLUDED

#include <string>

class entity;
class level;

struct screen_position {
	screen_position() : init(false), x(0), y(0), focus_x(0), focus_y(0),
	                    vertical_look(0), flip_rotate(0), coins(-1),
						shake_x_offset(0),shake_y_offset(0),shake_x_vel(0),shake_y_vel(0)
	{}
	bool init;
	int x, y;
	int focus_x, focus_y;
	int	shake_x_offset,shake_y_offset;
	int shake_x_vel,shake_y_vel;
	int vertical_look;
	int flip_rotate;
	int coins;
};

screen_position& last_draw_position();
void set_scene_title(const std::string& msg, int duration=50);
void draw_scene(const level& lvl, screen_position& pos, const entity* focus=NULL);

struct performance_data {
	int fps;
	int delay;
	int draw;
	int process;
	int cycle;
};

void draw_fps(const level& lvl, const performance_data& data);

#endif
