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
#ifndef NO_EDITOR
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <sstream>

#include "button.hpp"
#include "editor_dialogs.hpp"
#include "foreach.hpp"
#include "grid_widget.hpp"
#include "group_property_editor_dialog.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "load_level.hpp"
#include "raster.hpp"

namespace editor_dialogs
{

group_property_editor_dialog::group_property_editor_dialog(editor& e)
  : gui::dialog(graphics::screen_width() - 160, 160, 160, 440), editor_(e)
{
	group_ = e.get_level().editor_selection();
	init();
}

void group_property_editor_dialog::init()
{
	clear();

	using namespace gui;
	set_padding(20);

	if(group_.empty() == false) {
		std::cerr << "ADD BUTTON\n";
		add_widget(widget_ptr(new button(widget_ptr(new label("Group Objects", graphics::color_white())), boost::bind(&group_property_editor_dialog::group_objects, this))), 10, 10);
	}
}

void group_property_editor_dialog::set_group(const std::vector<entity_ptr>& group)
{
	group_ = group;
	init();
}

void group_property_editor_dialog::group_objects()
{
	editor_.group_selection();
}

}
#endif // !NO_EDITOR
