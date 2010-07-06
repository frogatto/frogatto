#include "iphone_controls.hpp"

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#include <SDL.h>

#include "geometry.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "texture.hpp"

namespace
{
	rect left_arrow, right_arrow, down_arrow, up_arrow, a_button, c_button, interact_button;

	const int underwater_circle_rad = 120;
	int underwater_circle_x = 150;
	int underwater_circle_y = 150; //changed to be relative to the bottom by setup_rects()

	bool is_underwater = false;
	bool can_interact = false;
	
	//This is to keep track of whether the rects above have been modified
	//by modify_rects() yet, to make them work on different resolutions
	bool done_setup_rects = false;
	static void setup_rects ()
	{
		if (done_setup_rects)
		{
			return;
		}
		done_setup_rects = true;
		
		int vw = preferences::virtual_screen_width();
		int vh = preferences::virtual_screen_height();
		
		left_arrow = rect(0, vh - 100, 10 + 55*2, 100);
		right_arrow = rect(244, vh - 100, 10 + 55*2, 100);
		down_arrow = rect(159, vh - 66, 34*2, 66);
		up_arrow = rect(159, vh - 184, 34*2, 55*2);
		a_button = rect(vw - 204, vh - 120, 50*2*2, 60*2);
//		b_button = rect(vw - 102, vh - 300, 50*2, 60*2);
		c_button = rect(vw - 104, vh - 240, 50*2, 60*2);
		
		interact_button = rect(vw - 320, vh - 120, 50*2, 60*2);
		
		underwater_circle_y = vh-underwater_circle_y;
	}
}

void translate_mouse_coords (int *x, int *y)
{
	if(preferences::screen_rotated()) {
		*x = preferences::actual_screen_width() - *x;
		std::swap(*x, *y);
	}
	
	if(preferences::virtual_screen_width() >
	   (preferences::screen_rotated() ? preferences::actual_screen_height() : preferences::actual_screen_width())) {
		*x *= 2;
		*y *= 2;
	}
}

void iphone_controls::set_underwater(bool value)
{
	is_underwater = value;
}

void iphone_controls::set_can_interact(bool value)
{
	can_interact = value;
}

bool iphone_controls::water_dir(float* xvalue, float* yvalue)
{
	setup_rects();
	for(int i = 0; i < SDL_GetNumMice(); i++) {
		int x, y;
		SDL_SelectMouse(i);
		Uint8 button_state = SDL_GetMouseState(&x, &y);
		if (button_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			translate_mouse_coords(&x, &y);

			const int dx = x - underwater_circle_x;
			const int dy = y - underwater_circle_y;

			const int distance = sqrt(dx*dx + dy*dy);

			if(distance > 0 && distance < underwater_circle_rad) {
				*xvalue = float(dx)/float(distance);
				*yvalue = float(dy)/float(distance);
				return true;
			}
		}
	}

	return false;
}
	
void iphone_controls::draw()
{
	if(!is_underwater) {
		return;
	}

	glColor4ub(128, 128, 128, 128);
	graphics::draw_circle(underwater_circle_x, underwater_circle_y, underwater_circle_rad);

	GLfloat x, y;
	if(water_dir(&x, &y)) {
		GLfloat varray[] = {
		  underwater_circle_x, underwater_circle_y,
		  underwater_circle_x + x*underwater_circle_rad, underwater_circle_y + y*underwater_circle_rad
		};

		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glColor4ub(255, 0, 0, 255);
		glVertexPointer(2, GL_FLOAT, 0, varray);
		glDrawArrays(GL_LINES, 0, (sizeof(varray)/sizeof(*varray))/2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
	}

	glColor4ub(255, 255, 255, 255);
}

bool iphone_controls::hittest_button(const rect& area)
{
	setup_rects();
	static graphics::texture tex(graphics::texture::get("gui/iphone_controls.png"));
	for(int i = 0; i < SDL_GetNumMice(); i++) {
		int x, y;
		SDL_SelectMouse(i);
		Uint8 button_state = SDL_GetMouseState(&x, &y);

		if(button_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			//rotate the coordinates
			translate_mouse_coords(&x, &y);

			const point mouse_pos(x, y);
			if(point_in_rect(mouse_pos, area)) {
				return true;
			}
		}
	}
	return false;
}

bool iphone_controls::up()
{
	if(is_underwater) {
		return false;
	}

	if(can_interact && hittest_button(interact_button)) {
		return true;
	}

	return hittest_button(up_arrow);
}

bool iphone_controls::down()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(down_arrow);
}

bool iphone_controls::left()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(left_arrow);
}

bool iphone_controls::right()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(right_arrow);
}

bool iphone_controls::attack()
{
	return false; //hittest_button(b_button);
}

bool iphone_controls::jump()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(a_button);
}

bool iphone_controls::tongue()
{
	return hittest_button(c_button);
}

#else // dummy functions for non-iPhone

void translate_mouse_coords (int *x, int *y) {}

void iphone_controls::draw() {}

void iphone_controls::set_underwater(bool value) {}
void iphone_controls::set_can_interact(bool value) {}

bool iphone_controls::water_dir(float* xvalue, float* yvalue) { return false; }

bool iphone_controls::up() {return false;}

bool iphone_controls::down() {return false;}

bool iphone_controls::left() {return false;}

bool iphone_controls::right() {return false;}

bool iphone_controls::attack() {return false;}

bool iphone_controls::jump() {return false;}

bool iphone_controls::tongue() {return false;}

#endif
