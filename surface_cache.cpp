
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "filesystem.hpp"
#include "surface_cache.hpp"
#include "SDL_image.h"

#include <iostream>
#include <map>

namespace graphics
{

namespace surface_cache
{

namespace {

	typedef std::map<std::string,surface> surface_map;
	surface_map cache;
	const std::string path = "./images/";
}

surface get(const std::string& key)
{
	surface_map::iterator i = cache.find(key);
	if(i != cache.end()) {
		return i->second;
	} else {
		const std::string fname = path + key;
		surface surf(IMG_Load(sys::find_file(fname).c_str()));
		std::cerr << "loading image '" << fname << "'\n";
		if(surf.get() == false) {
			std::cerr << "failed to load image '" << key << "'\n";
		}

		cache.insert(std::pair<std::string,surface>(key,surf));
		return surf;
	}
}

}

}
