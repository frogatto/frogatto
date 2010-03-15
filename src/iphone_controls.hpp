#ifndef IPHONE_CONTROLS_HPP_INCLUDED
#define IPHONE_CONTROLS_HPP_INCLUDED

#include <SDL.h>

class iphone_controls
{
public:
	static bool up();
	static bool down();
	static bool left();
	static bool right();
	static bool attack();
	static bool jump();
	static bool water_dir(float* x, float* y);

	static void set_underwater(bool value);

	static void draw();
	
private:
	static bool hittest_button (const SDL_Rect& rect, int button_x, int button_y);
};

#endif
