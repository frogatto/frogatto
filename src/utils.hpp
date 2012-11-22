
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef UTILS_HPP_INCLUDED
#define UTILS_HPP_INCLUDED

#ifdef _WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#endif

#include <algorithm>
#include <string>

std::string get_http_datetime();
int truncate_to_char(int value);
void write_autosave();
void toggle_fullscreen();

#ifdef _WINDOWS
int gettimeofday(struct timeval *tv, struct timezone2 *tz);
#endif

#endif
