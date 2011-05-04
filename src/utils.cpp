
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
#include "wml_node_fwd.hpp"
#include "wml_writer.hpp"
#include "filesystem.hpp"
#include "preferences.hpp"
#include "sound.hpp"

int truncate_to_char(int value) { return std::min(std::max(value, 0), 255); }

void write_autosave ()
{
	wml::node_ptr node = level::current().write();
	if(sound::current_music().empty() == false) {
		node->set_attr("music", sound::current_music());
	}
	
	sys::write_file(preferences::auto_save_file_path(), wml::output(node));
	sys::write_file(std::string(preferences::auto_save_file_path()) + ".stat", "1");
}
