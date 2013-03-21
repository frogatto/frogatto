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
#ifndef TOOLTIP_HPP_INCLUDED
#define TOOLTIP_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <string>

#include "color_utils.hpp"
#include "color_chart.hpp"
#include "font.hpp"

namespace gui {

struct tooltip_item
{
	explicit tooltip_item(const std::string& s, int fs=18, const SDL_Color& color=graphics::color_yellow(), const std::string& font="") 
		: font_size(fs), text(s), font_color(color), font_name(font)
	{}
	std::string text;
	int font_size;
	SDL_Color font_color;
	std::string font_name;
};

void set_tooltip(const boost::shared_ptr<tooltip_item>& str);
void remove_tooltip(const boost::shared_ptr<tooltip_item>& str);
void draw_tooltip();

}

#endif
