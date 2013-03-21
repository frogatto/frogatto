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
#ifndef CHARACTER_EDITOR_DIALOG_HPP_INCLUDED
#define CHARACTER_EDITOR_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR

#include <map>
#include <string>

#include "dialog.hpp"
#include "text_editor_widget.hpp"
#include "widget.hpp"

class editor;

namespace gui {
class border_widget;
}

namespace editor_dialogs
{

//editor dialog which displays the details of an object and allows editing it.
class character_editor_dialog : public gui::dialog
{
public:
	explicit character_editor_dialog(editor& e);
	void init();
	void set_character(int index);
	void select_category(const std::string& str);
private:
	void show_category_menu();

	void close_context_menu(int index);
	editor& editor_;
	std::string category_;
	gui::widget_ptr context_menu_;

	gui::widget_ptr generate_grid(const std::string& category);
	std::map<std::string, gui::widget_ptr> grids_;

	//the borders around each object, we set the currently selected one
	//to white, and all the others to transparent.
	std::map<std::string, std::vector<gui::border_widget_ptr> > grid_borders_;

	//the first object in each category
	std::map<std::string, int> first_obj_;

	boost::intrusive_ptr<gui::text_editor_widget> find_edit_;
};

typedef boost::intrusive_ptr<character_editor_dialog> character_editor_dialog_ptr;

}

#endif // NO_EDITOR
#endif
