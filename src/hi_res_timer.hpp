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
#ifndef HI_RES_TIMER_HPP_INCLUDED
#define HI_RES_TIMER_HPP_INCLUDED

#ifdef linux
#include <stdio.h>
#include <sys/time.h>
#endif

struct hi_res_timer {
	hi_res_timer(const char* str) {
#ifdef linux
		str_ = str;
		gettimeofday(&tv_, 0);
#endif
	};

#ifdef linux
	~hi_res_timer() {
		const int begin = (tv_.tv_sec%1000)*1000000 + tv_.tv_usec;
		gettimeofday(&tv_, 0);
		const int end = (tv_.tv_sec%1000)*1000000 + tv_.tv_usec;
		fprintf(stderr, "TIMER: %s: %dus\n", str_, (end - begin));
	}

	const char* str_;
	timeval tv_;
#endif
};

#endif
