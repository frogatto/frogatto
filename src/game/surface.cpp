
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "surface.hpp"

namespace graphics
{

namespace {

	SDL_PixelFormat& get_neutral_pixel_format()
	{
		static bool first_time = true;
		static SDL_PixelFormat format;

		if(first_time) {
			first_time = false;
			surface surf(SDL_CreateRGBSurface(SDL_SWSURFACE,1,1,32,SURFACE_MASK));
			format = *surf->format;
			format.palette = NULL;
		}

		return format;
	}

}

surface surface::convert_opengl_format() const
{
	return clone();
}

surface surface::clone() const
{
	return SDL_ConvertSurface(get(),&get_neutral_pixel_format(),
	                          SDL_SWSURFACE);
}

surface surface::create(int w, int h)
{
	return surface(SDL_CreateRGBSurface(SDL_SWSURFACE,w,h,32,0xFF0000,0xFF00,0xFF,0xFF000000));
}

}
