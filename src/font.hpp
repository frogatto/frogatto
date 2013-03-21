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
#ifndef FONT_HPP_INCLUDED
#define FONT_HPP_INCLUDED

#include <string>

#include "graphics.hpp"
#include "texture.hpp"

namespace font {

bool is_init();

struct manager {
	manager();
	~manager();
};

struct error {
};

graphics::texture render_text(const std::string& text,
                              const SDL_Color& color, int size, const std::string& font_name="");
graphics::texture render_text_uncached(const std::string& text,
                                       const SDL_Color& color, int size, const std::string& font_name="");

int char_width(int size, const std::string& fn="");
int char_height(int size, const std::string& fn="");

void reload_font_paths();

}

#endif
