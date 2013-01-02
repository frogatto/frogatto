#pragma once
#ifndef WIN_PROFILE_TIMER_HPP_INCLUDED
#define WIN_PROFILE_TIMER_HPP_INCLUDED

namespace profile {
	struct manager
	{
		LARGE_INTEGER frequency;
		LARGE_INTEGER t1, t2;
		double elapsedTime;

		manager()
		{
			QueryPerformanceFrequency(&frequency);
			QueryPerformanceCounter(&t1);
		}

		~manager()
		{
			QueryPerformanceCounter(&t2);
			elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
			std::cerr << elapsedTime << " ms\n";
		}
	};
}

#endif 
