#ifndef IPHONE_CONTROLS_HPP_INCLUDED
#define IPHONE_CONTROLS_HPP_INCLUDED

#include "graphics.hpp"

class rect;

void translate_mouse_coords (int *x, int *y);

class iphone_controls
{
public:
	static bool up();
	static bool down();
	static bool left();
	static bool right();
	static bool attack();
	static bool jump();
	static bool tongue();
	static bool water_dir(float* x, float* y);

	static void set_underwater(bool value);
	static void set_can_interact(bool value);
	static void set_on_platform(bool value);
	static void set_standing(bool value);

	static void draw();

	static void read_controls();
#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_BLACKBERRY) || defined(__ANDROID__)
	static void handle_event(const SDL_Event& event);
#endif

private:
	static bool hittest_button (const rect& r);
};

#endif
