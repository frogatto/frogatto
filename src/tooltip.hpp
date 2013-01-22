
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
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
