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

#include "dialog.hpp"
#include "editor_dialogs.hpp"
#include "foreach.hpp"
#include "grid_widget.hpp"
#include "label.hpp"
#include "load_level.hpp"
#include "raster.hpp"
#include "widget.hpp"

namespace {
void do_select_level(gui::dialog* d, const std::vector<std::string>& levels, int index, std::string* result) {
	if(index >= 0 && index < levels.size()) {
		d->close();
		*result = levels[index];
	}
}
}

std::string show_choose_level_dialog(const std::string& prompt)
{
	using namespace gui;
	dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
	d.add_widget(widget_ptr(new label(prompt, graphics::color_white(), 48)));

	std::string result;
	std::vector<std::string> levels = get_known_levels();
	gui::grid* grid = new gui::grid(1);
	grid->set_max_height(graphics::screen_height() - 80);
	grid->set_show_background(true);
	grid->allow_selection();

	grid->register_selection_callback(boost::bind(&do_select_level, &d, levels, _1, &result));
	foreach(const std::string& lvl, levels) {
		grid->add_col(widget_ptr(new label(lvl, graphics::color_white())));
	}

	d.add_widget(widget_ptr(grid));
	d.show_modal();
	return result;
}
#endif // NO_EDITOR

