
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef TRANSLATE_HPP_INCLUDED
#define TRANSLATE_HPP_INCLUDED

#include <string>

namespace i18n {

void add_translation(const std::string& from, const std::string& to);
const std::string& translate(const std::string& from);

}

#endif
