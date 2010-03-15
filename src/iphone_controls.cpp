#include "iphone_controls.hpp"

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#include <SDL.h>

#include "preferences.hpp"
#include "raster.hpp"
#include "texture.hpp"

namespace
{
	//The w/h of these rects are actually x+w and y+h
	SDL_Rect up_arrow = {106,2,156,37};
	SDL_Rect down_arrow = {106,39,156,74};
	SDL_Rect left_arrow = {106,76,141,126};
	SDL_Rect right_arrow = {143,76,178,126};
	SDL_Rect a_button = {2,62,52,122};
	SDL_Rect b_button = {2,2,52,60};

	const int underwater_circle_x = 120;
	const int underwater_circle_y = 520;

	bool is_underwater = false;
}

void iphone_controls::set_underwater(bool value)
{
	is_underwater = value;
}

bool iphone_controls::water_dir(float* xvalue, float* yvalue)
{
	for(int i = 0; i < SDL_GetNumMice(); i++) {
		int x, y;
		Uint8 button_state = SDL_GetMouseState(i, &x, &y);
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

bool iphone_controls::hittest_button (const SDL_Rect& rect, int button_x, int button_y)
{
	static graphics::texture tex(graphics::texture::get("gui/iphone_controls.png"));
	for (int i = 0; i < SDL_GetNumMice(); i++)
	{
		int x, y;
		Uint8 button_state = SDL_GetMouseState(i, &x, &y);
		if (button_state & SDL_BUTTON(SDL_BUTTON_LEFT))
		{
			//rotate the coordinates
			if (preferences::screen_rotated())
			{
				int tmp = x;
				x = y;
				y = preferences::actual_screen_width()-tmp;
			}
			int w = 3*(rect.w-rect.x)/2; // rect.w/h are really x+w and y+h
			int h = 3*(rect.h-rect.y)/2;
			//if (rect.w == 141) printf("Left rect: %i,%i,%i,%i - Mouse: %i,%i", button_x, button_y, w, h,  x*2, y*2);
			if (x*2 > button_x && x*2 < button_x+w*2 && y*2 > button_y && y*2 < button_y+h*2)
			{
				//if (!tex.is_alpha(rect.x+x-button_x/2, rect.y+y-button_y/2)) return true;
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

	return hittest_button(up_arrow, 46, preferences::virtual_screen_height()-305);
}

bool iphone_controls::down()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(down_arrow, 46, preferences::virtual_screen_height()-120);
}

bool iphone_controls::left()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(left_arrow, 18, preferences::virtual_screen_height()-236);
}

bool iphone_controls::right()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(right_arrow, 124, preferences::virtual_screen_height()-236);
}

bool iphone_controls::attack()
{
	return hittest_button(b_button, preferences::virtual_screen_width()-164, preferences::virtual_screen_height()-358);
}

bool iphone_controls::jump()
{
	if(is_underwater) {
		return false;
	}

	return hittest_button(a_button, preferences::virtual_screen_width()-164, preferences::virtual_screen_height()-184);
}

#else // dummy functions for non-iPhone

bool iphone_controls::up() {return false;}

bool iphone_controls::down() {return false;}

bool iphone_controls::left() {return false;}

bool iphone_controls::right() {return false;}

bool iphone_controls::attack() {return false;}

bool iphone_controls::jump() {return false;}

#endif
