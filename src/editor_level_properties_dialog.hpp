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
#ifndef EDITOR_LEVEL_PROPERTIES_DIALOG_HPP_INCLUDED
#define EDITOR_LEVEL_PROPERTIES_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR


#include <string>
#include <vector>

#include "dialog.hpp"
#include "text_editor_widget.hpp"

class editor;

namespace editor_dialogs
{

class editor_level_properties_dialog : public gui::dialog
{
public:
	explicit editor_level_properties_dialog(editor& e);
	void init();
private:
	void change_title(const gui::text_editor_widget_ptr editor);
	void change_background();
	void execute_change_background(const std::vector<std::string>& choices, int index);

	void change_next_level();
	void change_previous_level();

	editor& editor_;
	gui::widget_ptr context_menu_;
};

typedef boost::intrusive_ptr<editor_level_properties_dialog> editor_level_properties_dialog_ptr;

}

#endif // !NO_EDITOR
#endif
