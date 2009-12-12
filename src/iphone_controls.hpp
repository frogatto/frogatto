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
	
	static void process_event (const SDL_Event& event);
	
private:
	/*static bool up_;
	static bool down_;
	static bool left_;
	static bool right_;
	static bool attack_;
	static bool jump_;*/
};

#endif
