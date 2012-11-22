#ifndef SETTINGS_DIALOG_HPP_INCLUDED
#define SETTINGS_DIALOG_HPP_INCLUDED

#include "gui_section.hpp"
#include "graphics.hpp"

class settings_dialog
{
public:
	settings_dialog ();
	~settings_dialog ();
	
	bool handle_event (const SDL_Event& event); //returns whether the game should be paused
	void draw (bool in_speech_dialog) const;
	void reset ();
	
private:
	bool show_window_;
	bool menu_button_state_;
};

#endif
