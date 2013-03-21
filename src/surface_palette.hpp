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
#ifndef SURFACE_PALETTE_HPP_INCLUDED
#define SURFACE_PALETTE_HPP_INCLUDED

#include <string>

#include "color_utils.hpp"
#include "surface.hpp"

namespace graphics
{

int get_palette_id(const std::string& name);
const std::string& get_palette_name(int id);

surface map_palette(surface s, int palette);
color map_palette(const color& c, int palette);
SDL_Color map_palette(const SDL_Color& c, int palette);
}

#endif
