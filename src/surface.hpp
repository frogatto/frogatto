
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef SURFACE_HPP_INCLUDED
#define SURFACE_HPP_INCLUDED

#include <iostream>

#include "graphics.hpp"
#include "scoped_resource.hpp"

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SURFACE_MASK 0xFF,0xFF00,0xFF0000,0xFF000000
#define SURFACE_MASK_RGB 0xFF,0xFF00,0xFF0000,0x0
#else
#define SURFACE_MASK 0xFF000000,0xFF0000,0xFF00,0xFF
#define SURFACE_MASK_RGB 0xFF0000,0xFF00,0xFF,0x0
#endif

namespace graphics
{

struct surface
{
private:
	static void sdl_add_ref(SDL_Surface *surf)
	{
		if (surf != NULL)
			++surf->refcount;
	}

	struct free_sdl_surface {
		void operator()(SDL_Surface *surf) const
		{
			if (surf != NULL) {
				 SDL_FreeSurface(surf);
			}
		}
	};

	typedef util::scoped_resource<SDL_Surface*,free_sdl_surface> scoped_sdl_surface;
public:
	surface() : surface_(NULL)
	{}

	surface(SDL_Surface *surf) : surface_(surf)
	{
	}

	surface(const surface& o) : surface_(o.surface_.get())
	{
		sdl_add_ref(surface_.get());
	}

	static surface create(int w, int h);

	void assign(const surface& o)
	{
		SDL_Surface *surf = o.surface_.get();
		sdl_add_ref(surf); // need to be done before assign to avoid corruption on "a=a;"
		surface_.assign(surf);
	}

	surface& operator=(const surface& o)
	{
		assign(o);
		return *this;
	}

	operator SDL_Surface*() const { return surface_.get(); }

	SDL_Surface* get() const { return surface_.get(); }

	SDL_Surface* operator->() const { return surface_.get(); }

	void assign(SDL_Surface* surf) { surface_.assign(surf); }

	bool null() const { return surface_.get() == NULL; }

	surface convert_opengl_format() const;
	surface clone() const;

private:
	scoped_sdl_surface surface_;
};

inline bool operator==(const surface& a, const surface& b)
{
	return a.get() == b.get();
}

inline bool operator<(const surface& a, const surface& b)
{
	return a.get() < b.get();
}

}

#endif
