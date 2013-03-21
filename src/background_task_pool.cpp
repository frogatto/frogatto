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
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <vector>

#include "background_task_pool.hpp"
#include "foreach.hpp"
#include "thread.hpp"

namespace background_task_pool
{

namespace {

int next_task_id = 0;

struct task {
	boost::function<void()> job, on_complete;
	boost::shared_ptr<threading::thread> thread;
};

threading::mutex* completed_tasks_mutex = NULL;
std::vector<int> completed_tasks;

std::map<int, task> task_map;

void run_task(boost::function<void()> job, int task_id)
{
	job();
	threading::lock(*completed_tasks_mutex);
	completed_tasks.push_back(task_id);
}

}

manager::manager()
{
	completed_tasks_mutex = new threading::mutex;
}

manager::~manager()
{
	while(task_map.empty() == false) {
		pump();
	}
}

void submit(boost::function<void()> job, boost::function<void()> on_complete)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	task t = { job, on_complete, boost::shared_ptr<threading::thread>(new threading::thread("background_task", boost::bind(run_task, job, next_task_id))) };
#else
	task t = { job, on_complete, boost::shared_ptr<threading::thread>(new threading::thread(boost::bind(run_task, job, next_task_id))) };
#endif
	task_map[next_task_id] = t;
	++next_task_id;
}

void pump()
{
	std::vector<int> completed;
	{
		threading::lock(*completed_tasks_mutex);
		completed.swap(completed_tasks);
	}

	foreach(int t, completed) {
		task_map[t].on_complete();
		task_map.erase(t);
	}
}

}
