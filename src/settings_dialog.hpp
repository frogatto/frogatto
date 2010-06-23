#ifndef SETTINGS_DIALOG_HPP_INCLUDED
#define SETTINGS_DIALOG_HPP_INCLUDED

#include "gui_section.hpp"
#include <SDL.h>

class settings_dialog
{
public:
	settings_dialog ();
	~settings_dialog ();
	
	bool handle_event (const SDL_Event& event); //returns whether the game should be paused
	void draw () const;
	void reset ();
	
private:
	bool show_window_;
	bool menu_button_state_;
	const const_gui_section_ptr menu_button_normal_;
	const const_gui_section_ptr menu_button_down_;
};

#endif
