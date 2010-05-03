#ifndef SETTINGS_DIALOG_HPP_INCLUDED
#define SETTINGS_DIALOG_HPP_INCLUDED

#include <SDL.h>

class settings_dialog
{
public:
	settings_dialog();
	~settings_dialog();
	
	bool handle_event (const SDL_Event& event); //returns whether the game should be paused
	void draw () const;
	
private:
	bool show_window;
};

#endif
