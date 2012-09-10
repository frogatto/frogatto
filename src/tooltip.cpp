	
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "graphics.hpp"

#include "font.hpp"
#include "raster.hpp"
#include "tooltip.hpp"

namespace gui {

namespace {
boost::shared_ptr<tooltip_item> cur_tooltip;

graphics::texture& text() {
	static graphics::texture t;
	return t;
}
}

void set_tooltip(const boost::shared_ptr<tooltip_item>& tip)
{
	cur_tooltip = tip;
	text() = font::render_text(cur_tooltip->text, graphics::color_yellow(), cur_tooltip->font_size);
}

void remove_tooltip(const boost::shared_ptr<tooltip_item>& tip)
{
	if(tip == cur_tooltip) {
		cur_tooltip.reset();
		text() = graphics::texture();
	}
}

void draw_tooltip()
{
	if(!cur_tooltip) {
		return;
	}

	int mousex, mousey;
	SDL_GetMouseState(&mousex,&mousey);

	const int pad = 10;
	const int width = text().width() + pad*2;
	const int height = text().height() + pad*2;
	int x = mousex - width/2;
	int y = mousey - height;
	if(x < 0) {
		x = 0;
	}

	if(x > graphics::screen_width()-width) {
		x = graphics::screen_width()-width;
	}

	if(y < 0) {
		y = 0;
	}

	if(y > graphics::screen_height()-height) {
		y = graphics::screen_height()-height;
	}

	SDL_Rect rect = {x,y,width,height};
	graphics::draw_rect(rect, graphics::color_black(), 160);

	graphics::blit_texture(text(),x+pad,y+pad);
}

}
