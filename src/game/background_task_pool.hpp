#ifndef BACKGROUND_TASK_POOL_HPP_INCLUDED
#define BACKGROUND_TASK_POOL_HPP_INCLUDED

#include <boost/function.hpp>

namespace background_task_pool
{

struct manager {
	manager();
	~manager();
};

void pump();

void submit(boost::function<void()> job, boost::function<void()> on_complete);

}

#endif
