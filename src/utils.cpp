
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include <algorithm>
#include "utils.hpp"

#include "level.hpp"
#include "filesystem.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "sound.hpp"
#include "variant.hpp"

int truncate_to_char(int value) { return std::min(std::max(value, 0), 255); }

void write_autosave ()
{
	variant node = level::current().write();
	if(sound::current_music().empty() == false) {
		node.add_attr(variant("music"), variant(sound::current_music()));
	}
	
	sys::write_file(preferences::auto_save_file_path(), node.write_json());
	sys::write_file(std::string(preferences::auto_save_file_path()) + ".stat", "1");
}

void toggle_fullscreen ()
{
	preferences::set_fullscreen(!preferences::fullscreen());
	graphics::set_video_mode(graphics::screen_width(), graphics::screen_height());
}
