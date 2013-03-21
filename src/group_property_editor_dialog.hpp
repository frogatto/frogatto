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
#ifndef GROUP_PROPERTY_EDITOR_DIALOG_HPP_INCLUDED
#define GROUP_PROPERTY_EDITOR_DIALOG_HPP_INCLUDED
#ifndef NO_EDITOR

#include <string>
#include <vector>

#include "dialog.hpp"
#include "editor.hpp"
#include "entity.hpp"

namespace editor_dialogs
{

class group_property_editor_dialog : public gui::dialog
{
public:
	explicit group_property_editor_dialog(editor& e);
	void init();

	void set_group(const std::vector<entity_ptr>& group);
private:
	void group_objects();

	editor& editor_;
	std::vector<entity_ptr> group_;
};

typedef boost::intrusive_ptr<group_property_editor_dialog> group_property_editor_dialog_ptr;

}

#endif
#endif // !NO_EDITOR

