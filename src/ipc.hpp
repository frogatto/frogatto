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
#ifndef IPC_HPP_INCLUDED
#define IPC_HPP_INCLUDED

#if defined(UTILITY_IN_PROC)

#include <string>

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#endif

#if defined(_MSC_VER)
typedef HANDLE shared_sem_type;
#else
typedef sem_t* shared_sem_type;
#endif

namespace ipc
{
	namespace semaphore
	{
		bool in_use();
		void post();
		bool trywait();
		bool open(const std::string& sem_name);
		bool create(const std::string& sem_name, int initial_count);
	}
}

#endif

#endif