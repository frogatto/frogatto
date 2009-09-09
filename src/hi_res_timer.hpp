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
