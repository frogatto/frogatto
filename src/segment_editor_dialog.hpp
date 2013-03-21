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
#ifndef SEGMENT_EDITOR_DIALOG_HPP_INCLUDED
#define SEGMENT_EDITOR_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR

#include "dialog.hpp"

class editor;

namespace editor_dialogs
{

class segment_editor_dialog : public gui::dialog
{
public:
	explicit segment_editor_dialog(editor& e);
	void init();

	void set_segment(int num);

private:
	void set_segment_start_difficulty(int value);

	editor& editor_;
	int segment_;
};

}

#endif
#endif // !NO_EDITOR

