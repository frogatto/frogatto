#ifndef DRAW_SCENE_HPP_INCLUDED
#define DRAW_SCENE_HPP_INCLUDED

#include <string>

class entity;
class level;

struct screen_position {
	screen_position() : init(false), x(0), y(0), vertical_look(0),
	                    flip_rotate(0), coins(-1), score(-1),
						shake_x_offset(0),shake_y_offset(0),shake_x_vel(0),shake_y_vel(0)
	{}
	bool init;
	int x, y;
	int	shake_x_offset,shake_y_offset;
	int shake_x_vel,shake_y_vel;
	int vertical_look;
	int flip_rotate;
	int coins;
	int score;
};

screen_position& last_draw_position();
void set_scene_title(const std::string& msg);
void draw_scene(const level& lvl, screen_position& pos, const entity* focus=NULL);
void draw_statusbar(const level& lvl, screen_position& pos, const entity* focus=NULL);
void draw_fps(int fps, int delay);

#endif
