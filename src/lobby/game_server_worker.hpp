#pragma once
#ifndef GAME_SERVER_WORKER_HPP_INCLUDED
#define GAME_SERVER_WORKER_HPP_INCLUDED

#ifdef _MSC_VER
#include "targetver.h"
#endif

#include <cstdint>
#include <string>

#include "asserts.hpp"
#include "shared_data.hpp"

namespace game_server
{
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

	private:
		bool running_;
		shared_data& data_;
		int64_t polling_interval_;
		std::string server_address_;
		std::string server_port_;
		server_info si_;
	};
}

#endif
