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
#include <ctype.h>
#include <iostream>

#include "font.hpp"
#include "foreach.hpp"
#include "dialog.hpp"
#include "raster.hpp"
#include "options_dialog.hpp"

namespace {
	
	void draw_frame(const rect& r)
	{
		const SDL_Color border = { 0xa2, 0x64, 0x76, 0xff };
		const SDL_Color bg = { 0xbe, 0xa2, 0x8f, 0xff };
		
		const int Border = 4;
		const int Padding = 10;
		rect border_rect(r.x() - Padding - Border, r.y() - Padding - Border, r.w() + + Padding*2 + Border*2, r.h() + Padding*2 + Border*2);
		graphics::draw_rect(border_rect.sdl_rect(), border);
		rect back_rect(r.x() - Padding, r.y() - Padding, r.w() + Padding*2, r.h() + Padding*2);
		graphics::draw_rect(back_rect.sdl_rect(), bg);
	}
	
}


void options_dialog::draw() const
{
	draw_frame( rect(x(),y(),width(),height()) );
}

	
options_dialog::options_dialog(int x, int y, int w, int h)
: dialog(x,y,w,h)
{

}

void options_dialog::handle_draw() const
{
	/*if(clear_bg()) {
		SDL_Rect rect = {x(),y(),width(),height()};
		SDL_Color col = {0,0,0,0};
		graphics::draw_rect(rect,col,196);
		
		//fade effect for fullscreen dialogs
		if(bg_.valid()) {
			if(bg_alpha_ > 0.25) {
				bg_alpha_ -= 0.05;
			}
			glColor4f(1.0, 1.0, 1.0, bg_alpha_);
			graphics::blit_texture(bg_, x(), y(), width(), height(), 0.0, 0.0, 1.0, 1.0, 0.0);
			glColor4f(1.0, 1.0, 1.0, 1.0);
		}
	}*/
	draw_frame( rect(x(),y(),width(),height()) );
	//handle_draw_children();
}
