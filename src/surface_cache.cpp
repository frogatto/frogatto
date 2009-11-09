
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "concurrent_cache.hpp"
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

	typedef concurrent_cache<std::string,surface> surface_map;
	surface_map cache;
	const std::string path = "./images/";
}

surface get(const std::string& key)
{
	surface surf = cache.get(key);
	if(surf.null()) {
		const std::string fname = path + key;
		surf = surface(IMG_Load(sys::find_file(fname).c_str()));
		std::cerr << "loading image '" << fname << "'\n";
		if(surf.get() == false) {
			std::cerr << "failed to load image '" << key << "'\n";
			return surface();
		}

		cache.put(key,surf);
	}

	return surf;
}

void clear_unused()
{
	surface_map::lock lck(cache);
	std::map<std::string, surface>& map = lck.map();
	std::map<std::string, surface>::iterator i = map.begin();
	while(i != map.end()) {
		std::cerr << "CACHE REF " << i->first << " -> " << i->second->refcount << "\n";
		if(i->second->refcount == 1) {
			std::cerr << "CACHE FREE " << i->first << "\n";
			map.erase(i++);
		} else {
			++i;
		}
	}

	std::cerr << "CACHE ITEMS: " << map.size() << "\n";
}

void clear()
{
	cache.clear();
}

}

}
