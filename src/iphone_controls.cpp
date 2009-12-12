#include "iphone_controls.hpp"

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#include <SDL.h>

bool iphone_controls::up()
{
	for (int i = 0; i < SDL_GetNumMice(); i++)
	{
		int x, y;
		Uint8 button_state = SDL_GetMouseState(i, &x, &y);
		int tmp = x;
		x = y;
		y = 320-tmp;
		if (button_state & SDL_BUTTON(SDL_BUTTON_LEFT) && y < 40)
		{
			return true;
		}
	}
	return false;
}

bool iphone_controls::down()
{
	for (int i = 0; i < SDL_GetNumMice(); i++)
	{
		int x, y;
		Uint8 button_state = SDL_GetMouseState(i, &x, &y);
		int tmp = x;
		x = y;
		y = 320-tmp;
		if (button_state & SDL_BUTTON(SDL_BUTTON_LEFT) && y > 280)
		{
			return true;
		}
	}
	return false;
}

bool iphone_controls::left()
{
	for (int i = 0; i < SDL_GetNumMice(); i++)
	{
		int x, y;
		Uint8 button_state = SDL_GetMouseState(i, &x, &y);
		int tmp = x;
		x = y;
		y = 320-tmp;
		if (button_state & SDL_BUTTON(SDL_BUTTON_LEFT) && x < 240)
		{
			return true;
		}
	}
	return false;
}

bool iphone_controls::right()
{
	for (int i = 0; i < SDL_GetNumMice(); i++)
	{
		int x, y;
		Uint8 button_state = SDL_GetMouseState(i, &x, &y);
		int tmp = x;
		x = y;
		y = 320-tmp;
		if (button_state & SDL_BUTTON(SDL_BUTTON_LEFT) && x > 240)
		{
			return true;
		}
	}
	return false;
}

bool iphone_controls::attack() {return false;}

bool iphone_controls::jump()
{
	for (int i = 0; i < SDL_GetNumMice(); i++)
	{
		int x, y;
		Uint8 button_state = SDL_GetMouseState(i, &x, &y);
		int tmp = x;
		x = y;
		y = 320-tmp;
		if (button_state & SDL_BUTTON(SDL_BUTTON_LEFT) && y > 40 && y < 80)
		{
			return true;
		}
	}
	return false;
}

#else // dummy functions for non-iPhone

bool iphone_controls::up() {return false;}

bool iphone_controls::down() {return false;}

bool iphone_controls::left() {return false;}

bool iphone_controls::right() {return false;}

bool iphone_controls::attack() {return false;}

bool iphone_controls::jump() {return false;}

#endif
