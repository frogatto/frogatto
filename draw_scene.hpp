#ifndef DRAW_SCENE_HPP_INCLUDED
#define DRAW_SCENE_HPP_INCLUDED

#include <string>

class entity;
class level;

struct screen_position {
	screen_position() : init(false), x(0), y(0), vertical_look(0),
	                    flip_rotate(0)
	{}
	bool init;
	int x, y;
	int vertical_look;
	int flip_rotate;
};

screen_position& last_draw_position();
void set_scene_title(const std::string& msg);
void draw_scene(const level& lvl, screen_position& pos, const entity* focus=NULL);

#endif
