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
