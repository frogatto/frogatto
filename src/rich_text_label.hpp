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
#ifndef RICH_TEXT_LABEL_HPP_INCLUDED
#define RICH_TEXT_LABEL_HPP_INCLUDED

#include <string>
#include <vector>

#include "formula_callable.hpp"
#include "scrollable_widget.hpp"
#include "widget.hpp"

namespace gui
{

class rich_text_label : public scrollable_widget
{
public:
	rich_text_label(const variant& v, game_logic::formula_callable* e);

	std::vector<widget_ptr> get_children() const;
private:

	void handle_process();
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& v);

	std::vector<std::vector<widget_ptr> > children_;
};

}

#endif
