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
