#pragma once
#ifndef GAME_SERVER_WORKER_HPP_INCLUDED
#define GAME_SERVER_WORKER_HPP_INCLUDED

#ifdef _MSC_VER
#include "targetver.h"
#endif

#include <boost/shared_ptr.hpp>
#include <cstdint>
#include <string>
#include <json_spirit.h>

#include "asserts.hpp"
#include "queue.hpp"
#include "shared_data.hpp"

namespace game_server
{
	typedef queue::queue<std::string> string_queue;
	typedef boost::shared_ptr<string_queue> string_queue_ptr;
	struct message
	{
		std::string msg;
		string_queue_ptr reply;
	};

	typedef queue::queue<message> message_queue;
	typedef boost::shared_ptr<message> message_ptr;
	typedef boost::shared_ptr<message_queue> message_queue_ptr;

	class worker
	{
	public:
		explicit worker(int64_t polling_interval, 
			shared_data& data, 
			const std::string& addr, 
			const std::string& port);
		virtual ~worker()
		{}
		void operator()();
		void abort();
		void process_game(const json_spirit::mObject& obj);
		double server_load() const { return server_load_; }
		message_queue_ptr get_queue() { return q_; }
		const server_info& get_server_info() const { return si_; }

		static worker& get_server_from_game_type(const std::string& game_type);
		static void get_server_info(json_spirit::mObject* obj);
	private:
		bool running_;
		shared_data& data_;
		message_queue_ptr q_;
		int64_t polling_interval_;
		std::string server_address_;
		std::string server_port_;
		server_info si_;
		double server_load_;
	};
}

#endif
