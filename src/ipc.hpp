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
		void post();
		bool trywait();
		bool open(const std::string& sem_name);
		bool create(const std::string& sem_name, int initial_count);
	}
}

#endif

#endif