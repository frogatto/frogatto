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
	task t = { job, on_complete, boost::shared_ptr<threading::thread>(new threading::thread(boost::bind(run_task, job, next_task_id))) };
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
