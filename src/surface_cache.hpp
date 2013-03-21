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
#ifndef SURFACE_CACHE_HPP_INCLUDED
#define SURFACE_CACHE_HPP_INCLUDED

#include <stdint.h>
#include <string>
#include <vector>

#include "data_blob.hpp"
#include "surface.hpp"

namespace graphics
{

struct load_image_error
{
};

namespace surface_cache
{

surface get(const std::string& key);
surface get_no_cache(const std::string& key, std::string* fname=0);
surface get_no_cache(data_blob_ptr blob);
void invalidate_modified(std::vector<std::string>* keys);
void clear_unused();
void clear();

}

}

#endif
