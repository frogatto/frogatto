
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "asserts.hpp"
#include "concurrent_cache.hpp"
#include "filesystem.hpp"
#include "module.hpp"
#include "surface_cache.hpp"
#if defined(__MACOSX__) || TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
	#include <SDL_image.h>
#else	
	#include <SDL/SDL_image.h>
#endif

#include <assert.h>
#include <iostream>
#include <map>

namespace graphics
{

namespace surface_cache
{

namespace {

typedef concurrent_cache<std::string,surface> surface_map;
surface_map& cache() {
	static surface_map c;
	return c;
}

const std::string path = "./images/";
}

surface get(const std::string& key)
{
	surface surf = cache().get(key);
	if(surf.null()) {
		surf = get_no_cache(key);
		cache().put(key,surf);
	}

	return surf;
}

surface get_no_cache(const std::string& key)
{
	const std::string fname = path + key;
	surface surf = surface(IMG_Load(module::map_file(fname).c_str()));
	//std::cerr << "loading image '" << fname << "'\n";
	if(surf.get() == false || surf->w == 0) {
		std::cerr << "failed to load image '" << key << "'\n";
		throw load_image_error();
	}

	//std::cerr << "IMAGE SIZE: " << (surf->w*surf->h) << "\n";
	return surf;
}

void clear_unused()
{
	surface_map::lock lck(cache());
	std::map<std::string, surface>& map = lck.map();
	std::map<std::string, surface>::iterator i = map.begin();
	while(i != map.end()) {
		//std::cerr << "CACHE REF " << i->first << " -> " << i->second->refcount << "\n";
		if(i->second->refcount == 1) {
			//std::cerr << "CACHE FREE " << i->first << "\n";
			map.erase(i++);
		} else {
			++i;
		}
	}

	//std::cerr << "CACHE ITEMS: " << map.size() << "\n";
}

void clear()
{
	cache().clear();
}

}

}
