#include "iphone_controls.hpp"

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#include <SDL.h>

#include "geometry.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "texture.hpp"

namespace
{
	const rect left_arrow(10, 640 - 84, 55*2, 34*2);
	const rect right_arrow(184, 640 - 84, 55*2, 34*2);
	const rect down_arrow(119, 640 - 66, 34*2, 42*2);
	const rect up_arrow(119, 640 - 147, 34*2, 55*2);
	const rect a_button(960 - 204, 640 - 120, 50*2, 60*2);
	const rect b_button(960 - 102, 640 - 240, 50*2, 60*2);
	const rect c_button(960 - 102, 640 - 360, 50*2, 60*2);

	const rect interact_button(960 - 300 - 30, 640 - 130, 50*2 + 60, 60*2);

	const int underwater_circle_x = 120;
	const int underwater_circle_y = 520;

	bool is_underwater = false;
	bool can_interact = false;
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
	for(int i = 0; i < SDL_GetNumMice(); i++) {
		int x, y;
		SDL_SelectMouse(i);
		Uint8 button_state = SDL_GetMouseState(&x, &y);
		if (button_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			if (preferences::screen_rotated()) {
				int tmp = x;
				x = y;
				y = preferences::actual_screen_width()-tmp;
			}

			const int dx = x - underwater_circle_x/2;
			const int dy = y - underwater_circle_y/2;

			const int distance = sqrt(dx*dx + dy*dy);

			if(distance > 0 && distance < 90) {
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
	graphics::draw_circle(underwater_circle_x, underwater_circle_y, 90);

	GLfloat x, y;
	if(water_dir(&x, &y)) {
		GLfloat varray[] = {
		  underwater_circle_x, underwater_circle_y,
		  underwater_circle_x + x*90, underwater_circle_y + y*90
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
	static graphics::texture tex(graphics::texture::get("gui/iphone_controls.png"));
	for(int i = 0; i < SDL_GetNumMice(); i++) {
		int x, y;
		SDL_SelectMouse(i);
		Uint8 button_state = SDL_GetMouseState(&x, &y);
		if(button_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			//rotate the coordinates
			if(preferences::screen_rotated()) {
				x = preferences::actual_screen_width() - x;
				std::swap(x, y);
			}

			const point mouse_pos(x*2, y*2);
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
	return hittest_button(b_button);
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
	if(is_underwater) {
		return false;
	}

	return hittest_button(c_button);
}

#else // dummy functions for non-iPhone

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
