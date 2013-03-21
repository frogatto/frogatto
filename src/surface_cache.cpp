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
#include "asserts.hpp"
#include "concurrent_cache.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "module.hpp"
#include "preferences.hpp"
#include "surface_cache.hpp"
#if defined(__MACOSX__) || TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR || defined(TARGET_BLACKBERRY) || defined(_WIN32) || defined(__ANDROID__)
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

struct CacheEntry {
	surface surf;
	std::string fname;
	int64_t mod_time;
};

typedef concurrent_cache<std::string,CacheEntry> surface_map;
surface_map& cache() {
	static surface_map c;
	return c;
}

const std::string path = "./images/";
}

void invalidate_modified(std::vector<std::string>* keys_modified)
{
	std::vector<std::string> keys = cache().get_keys();
	foreach(const std::string& k, keys) {
		CacheEntry entry = cache().get(k);
		const int64_t mod_time = sys::file_mod_time(entry.fname);
		if(mod_time != entry.mod_time) {
			cache().erase(k);
			if(keys_modified) {
				keys_modified->push_back(k);
			}
		}
	}
}

surface get(const std::string& key)
{
	surface surf = cache().get(key).surf;
	if(surf.null()) {
		CacheEntry entry;
		surf = entry.surf = get_no_cache(key, &entry.fname);
		if(entry.fname.empty() == false) {
			entry.mod_time = sys::file_mod_time(entry.fname);
		}

		cache().put(key,entry);
	}

	return surf;
}

surface get_no_cache(data_blob_ptr blob)
{
	ASSERT_LOG(blob != NULL, "Invalid data_blob in surface::get_no_cache");
	surface surf = surface(IMG_Load_RW(blob->get_rw_ops(), 0));
	if(surf.get() == false || surf->w == 0) {
		std::cerr << "failed to load image '" << (*blob)() << "'\n";
		throw load_image_error();
	}
	return surf;
}

surface get_no_cache(const std::string& key, std::string* full_filename)
{
	std::string fname = path + key;
#if defined(__ANDROID__)
	if(fname[0] == '.' && fname[1] == '/') {
		fname = fname.substr(2);
	}
	SDL_RWops *rw = sys::read_sdl_rw_from_asset(module::map_file(fname).c_str());
	surface surf;
	if(rw) {
		surf = surface(IMG_Load_RW(rw,1));
	} else {
		surf = surface(IMG_Load(module::map_file(fname).c_str()));
	}
#else
	surface surf;
	if(key.empty() == false && key[0] == '#') {
		const std::string fname = std::string(preferences::user_data_path()) + "/tmp_images/" + std::string(key.begin()+1, key.end());
		surf = surface(IMG_Load(fname.c_str()));
		if(full_filename) {
			*full_filename = fname;
		}
	} else if(sys::file_exists(key)) {
		surf = surface(IMG_Load(key.c_str()));
		if(full_filename) {
			*full_filename = key;
		}
	} else {
		surf = surface(IMG_Load(module::map_file(fname).c_str()));
		if(full_filename) {
			*full_filename = module::map_file(fname);
		}
	}
#endif // ANDROID
	//std::cerr << "loading image '" << fname << "'\n";
	if(surf.get() == false || surf->w == 0) {
		if(key != "") {
			std::cerr << "failed to load image '" << key << "'\n";
		}
		throw load_image_error();
	}

	//std::cerr << "IMAGE SIZE: " << (surf->w*surf->h) << "\n";
	return surf;
}

void clear_unused()
{
	surface_map::lock lck(cache());
	std::map<std::string, CacheEntry>& map = lck.map();
	std::map<std::string, CacheEntry>::iterator i = map.begin();
	while(i != map.end()) {
		//std::cerr << "CACHE REF " << i->first << " -> " << i->second->refcount << "\n";
		if(i->second.surf->refcount == 1) {
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
