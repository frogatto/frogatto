/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
