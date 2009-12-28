#include "iphone_controls.hpp"

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#include <SDL.h>

#include "texture.hpp"
#include "preferences.hpp"

namespace
{
	//The w/h of these rects are actually x+w and y+h
	SDL_Rect up_arrow = {106,2,156,37};
	SDL_Rect down_arrow = {106,39,156,74};
	SDL_Rect left_arrow = {106,76,141,126};
	SDL_Rect right_arrow = {143,76,178,126};
	SDL_Rect a_button = {2,62,52,122};
	SDL_Rect b_button = {2,2,52,60};
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
			int w = rect.w-rect.x; // rect.w/h are really x+w and y+h
			int h = rect.h-rect.y;
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
	return hittest_button(up_arrow, 46, preferences::virtual_screen_height()-284);
}

bool iphone_controls::down()
{
	return hittest_button(down_arrow, 46, preferences::virtual_screen_height()-178);
}

bool iphone_controls::left()
{
	return hittest_button(left_arrow, 0, preferences::virtual_screen_height()-246);
}

bool iphone_controls::right()
{
	return hittest_button(right_arrow, 114, preferences::virtual_screen_height()-246);
}

bool iphone_controls::attack()
{
	return hittest_button(b_button, 8, preferences::virtual_screen_height()-124);
}

bool iphone_controls::jump()
{
	return hittest_button(a_button, preferences::virtual_screen_width()-108, preferences::virtual_screen_height()-124);
}

#else // dummy functions for non-iPhone

bool iphone_controls::up() {return false;}

bool iphone_controls::down() {return false;}

bool iphone_controls::left() {return false;}

bool iphone_controls::right() {return false;}

bool iphone_controls::attack() {return false;}

bool iphone_controls::jump() {return false;}

#endif
