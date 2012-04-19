/* $Id: thread.cpp 31858 2009-01-01 10:27:41Z mordante $ */
/*
   Copyright (C) 2003 - 2009 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2
   or at your option any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include <boost/scoped_ptr.hpp>

#include <iostream>
#include <vector>

#include "thread.hpp"

namespace {

std::vector<SDL_Thread*> detached_threads;

}

namespace threading {

manager::~manager()
{
	for(std::vector<SDL_Thread*>::iterator i = detached_threads.begin(); i != detached_threads.end(); ++i) {
		SDL_WaitThread(*i,NULL);
	}
}

namespace {

int call_boost_function(void* arg)
{
	boost::scoped_ptr<boost::function<void()> > fn((boost::function<void()>*)arg);
	(*fn)();
	return 0;
}

}

thread::thread(boost::function<void()> fn) : fn_(fn), thread_(SDL_CreateThread(call_boost_function, new boost::function<void()>(fn_)))
{}

thread::~thread()
{
	join();
}

void thread::kill()
{
	if(thread_ != NULL) {
		SDL_KillThread(thread_);
		thread_ = NULL;
	}
}

void thread::join()
{
	if(thread_ != NULL) {
		SDL_WaitThread(thread_,NULL);
		thread_ = NULL;
	}
}

void thread::detach()
{
	detached_threads.push_back(thread_);
	thread_ = NULL;
}

mutex::mutex() : m_(SDL_CreateMutex())
{}

mutex::~mutex()
{
	SDL_DestroyMutex(m_);
}

mutex::mutex(const mutex&) : m_(SDL_CreateMutex())
{}

const mutex& mutex::operator=(const mutex&)
{
	return *this;
}

lock::lock(const mutex& m) : m_(m)
{
	SDL_mutexP(m_.m_);
}

lock::~lock()
{
	SDL_mutexV(m_.m_);
}

condition::condition() : cond_(SDL_CreateCond())
{}

condition::~condition()
{
	SDL_DestroyCond(cond_);
}

bool condition::wait(const mutex& m)
{
	return SDL_CondWait(cond_,m.m_) == 0;
}

condition::WAIT_TIMEOUT_RESULT condition::wait_timeout(const mutex& m, unsigned int timeout)
{
	const int res = SDL_CondWaitTimeout(cond_,m.m_,timeout);
	switch(res) {
		case 0: return THREAD_WAIT_OK;
		case SDL_MUTEX_TIMEDOUT: return THREAD_WAIT_TIMEOUT;
		default:
			 std::cerr << "SDL_CondWaitTimeout: " << SDL_GetError() << "\n";
			return THREAD_WAIT_ERROR;
	}
}

bool condition::notify_one()
{
	if(SDL_CondSignal(cond_) < 0) {
		std::cerr << "SDL_CondSignal: " << SDL_GetError() << "\n";
		return false;
	}

	return true;
}

bool condition::notify_all()
{
	if(SDL_CondBroadcast(cond_) < 0) {
		std::cerr << "SDL_CondBroadcast: " << SDL_GetError() << "\n";
		return false;
	}
	return true;
}

}
