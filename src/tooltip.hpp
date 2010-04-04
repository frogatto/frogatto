
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef TOOLTIP_HPP_INCLUDED
#define TOOLTIP_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <string>

namespace gui {

void set_tooltip(const boost::shared_ptr<std::string>& str);
void remove_tooltip(const boost::shared_ptr<std::string>& str);
void draw_tooltip();

}

#endif
