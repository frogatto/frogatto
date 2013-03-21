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
#ifndef CLIPBOARD_HPP_INCLUDED
#define CLIPBOARD_HPP_INCLUDED

#include <string>
#include "graphics.hpp"

void init_clipboard();

/**
 * Copies text to the clipboard.
 *
 * @param text         The text to copy.
 * @param mouse        Is the selection done by the mouse? On UNIX systems there
 *                     are multiple clipboards and the mouse selection uses a
 *                     different clipboard. Ignored on other systems.
 */
void copy_to_clipboard(const std::string& text, const bool mouse);

/**
 * Copies text from the clipboard.
 *
 * @param mouse        Is the pasting done by the mouse?
 *
 * @returns            String on clipbaord.
 */
std::string copy_from_clipboard(const bool mouse);

bool clipboard_handle_event(const SDL_Event& ev);

//if the clipboard has X-style copy paste using the mouse only.
bool clipboard_has_mouse_area();

#endif
