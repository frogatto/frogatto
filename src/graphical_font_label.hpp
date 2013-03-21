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
#ifndef GRAPHICAL_FONT_LABEL_HPP_INCLUDED
#define GRAPHICAL_FONT_LABEL_HPP_INCLUDED

#include "graphical_font.hpp"
#include "widget.hpp"

namespace gui {

class graphical_font_label : public widget
{
public:
	graphical_font_label(const std::string& text, const std::string& font, int size=1);
	graphical_font_label(const variant& v, game_logic::formula_callable* e);

	void set_text(const std::string& text);
	void reset_text_dimensions();
	
	void set_value(const std::string& key, const variant& v);
	variant get_value(const std::string& key) const;
private:
	void handle_draw() const;

	std::string text_;
	const_graphical_font_ptr font_;
	int size_;
};

typedef boost::intrusive_ptr<graphical_font_label> graphical_font_label_ptr;

}

#endif
