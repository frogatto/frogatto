#pragma once
#ifndef QUEUE_HPP_INCLUDED
#define QUEUE_HPP_INCLUDED

#include <cstdint>
#include <queue>
#include <boost/thread.hpp>
#include <boost/chrono/chrono.hpp>

namespace queue 
{
	template<class T>
	class queue
	{
	public:
		queue()
		{}
		void push(T const& data)
		{
			boost::mutex::scoped_lock lock(guard_);
			q_.push(data);
			lock.unlock();
			cv_.notify_one();
		}

		bool empty() const
		{
			boost::mutex::scoped_lock lock(guard_);
			return q_.empty();
		}

		bool try_pop(T& popped_value)
		{
			boost::mutex::scoped_lock lock(guard_);
			if(q_.empty()) {
				return false;
			}
        
			popped_value = q_.front();
			q_.pop();
			return true;
		}

		bool wait_and_pop(T& popped_value, int64_t interval = 0)
		{
			using namespace boost::chrono;
			boost::mutex::scoped_lock lock(guard_);
			system_clock::time_point time_limit = system_clock::now() + milliseconds(interval);
			while(q_.empty()) {
				if(interval) {
					if(cv_.wait_until<system_clock, system_clock::duration>(lock, time_limit) == boost::cv_status::timeout) {
						return false;
					}
				} else {
					cv_.wait(lock);
				}
			}
        
			popped_value = q_.front();
			q_.pop();
			return true;
		}

	private:
		std::queue<T> q_;
		mutable boost::mutex guard_;
		boost::condition_variable cv_;
		queue(const queue&)
		{}
	};
}

#endif