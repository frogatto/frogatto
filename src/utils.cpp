
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
#include <ctime>
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

std::string get_http_datetime() 
{
	time_t rawtime;
	char buffer[128];
	time(&rawtime);
#if defined(_WINDOWS)
	struct tm timeinfo;
	gmtime_s(&timeinfo, &rawtime);
	// RFC 8022 format
	strftime(buffer, 80, "%a, %d %b %Y %X GMT", &timeinfo);
#else
	struct tm* timeinfo;
	timeinfo = gmtime(&rawtime);
	// RFC 8022 format
	strftime(buffer, 80, "%a, %d %b %Y %X GMT", timeinfo);
#endif
	return std::string(buffer);
}

#ifdef _WINDOWS
const __int64 DELTA_EPOCH_IN_MICROSECS= 11644473600000000;

struct timezone2 
{
  __int32  tz_minuteswest; /* minutes W of Greenwich */
  bool  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone2 *tz)
{
	if(tv) {
		FILETIME ft;
		__int64 tmpres = 0;
		ZeroMemory(&ft,sizeof(ft));
		GetSystemTimeAsFileTime(&ft);

		tmpres = ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres /= 10;  /*convert into microseconds*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS; 
		tv->tv_sec = (__int32)(tmpres * 0.000001);
		tv->tv_usec =(tmpres % 1000000);
	}

    //_tzset(),don't work properly, so we use GetTimeZoneInformation
	if(tz) {
		TIME_ZONE_INFORMATION tz_winapi;
		ZeroMemory(&tz_winapi, sizeof(tz_winapi));
		int rez = GetTimeZoneInformation(&tz_winapi);
		tz->tz_dsttime = (rez == 2) ? true : false;
		tz->tz_minuteswest = tz_winapi.Bias + ((rez == 2) ? tz_winapi.DaylightBias : 0);
	}
	return 0;
}
#endif 
