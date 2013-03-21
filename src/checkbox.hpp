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
#ifndef CHECKBOX_HPP_INCLUDED
#define CHECKBOX_HPP_INCLUDED

#include "button.hpp"

#include <string>

namespace gui {

class checkbox : public virtual button
{
public:
	checkbox(const std::string& label, bool checked, boost::function<void(bool)> onclick, BUTTON_RESOLUTION button_resolution=BUTTON_SIZE_NORMAL_RESOLUTION);
	checkbox(widget_ptr label, bool checked, boost::function<void(bool)> onclick, BUTTON_RESOLUTION button_resolution=BUTTON_SIZE_NORMAL_RESOLUTION);
	checkbox(const variant& v, game_logic::formula_callable* e);
protected:
	virtual variant get_value(const std::string& key) const;
private:
	void on_click();

	std::string label_;
	widget_ptr label_widget_;
	boost::function<void(bool)> onclick_;
	bool checked_;
	game_logic::formula_ptr click_handler_;
	void click(bool checked);
	int hpadding_;
};

typedef boost::intrusive_ptr<checkbox> checkbox_ptr;

}

#endif
