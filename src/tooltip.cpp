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
	text() = font::render_text(cur_tooltip->text, cur_tooltip->font_color, cur_tooltip->font_size, cur_tooltip->font_name);
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
