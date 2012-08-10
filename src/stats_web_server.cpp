#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <iostream>

#if !defined(_WINDOWS)
#include <sys/time.h>
#endif

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "json_parser.hpp"
#include "stats_server.hpp"
#include "stats_web_server.hpp"
#include "string_utils.hpp"
#include "utils.hpp"
#include "variant.hpp"

std::string global_debug_str;

web_server::web_server(boost::asio::io_service& io_service, int port)
	: http::web_server(io_service, port), timer_(io_service), nheartbeat_(0)
{
	heartbeat();
}

void web_server::handle_post(socket_ptr socket, variant doc, const http::environment& env)
{
	static const variant TypeVariant("type");
	const std::string& type = doc[TypeVariant].as_string();
	if(type == "stats") {
		process_stats(doc);
	} else if(type == "upload_table_definitions") {
		//TODO: add authentication to get info about the user
		//and make sure they have permission to update this module.
		const std::string& module = doc[variant("module")].as_string();
		init_tables_for_module(module, doc[variant("definition")]);

		try {
			send_msg(socket, "text/json", "{ \"status\": \"ok\" }", "");
			sys::write_file("stats-definitions.json", get_tables_definition().write_json());
		} catch(validation_failure_exception& e) {
			std::map<variant,variant> msg;
			msg[variant("status")] = variant("error");
			msg[variant("message")] = variant(e.msg);
			send_msg(socket, "text/json", variant(&msg).write_json(), "");
		}

		return;
	}
	disconnect(socket);
}


void web_server::handle_get(socket_ptr socket, 
	const std::string& url, 
	const std::map<std::string, std::string>& args)
{
	std::map<std::string, std::string>::const_iterator it = args.find("type");
	if(it != args.end() && it->second == "status") {
		std::map<variant,variant> m;
		const std::map<std::string,std::string> errors = get_stats_errors();
		for(std::map<std::string,std::string>::const_iterator i = errors.begin(); i != errors.end(); ++i) {
			std::string msg = i->second;
			if(msg.empty()) {
				msg = "OK";
			}

			m[variant(i->first)] = variant(msg);
		}

		send_msg(socket, "text/json", variant(&m).write_json(), "");
		return;
	}

	variant value = get_stats(args.count("version") ? args.find("version")->second : "", 
		args.count("module") ? args.find("module")->second : "",
		args.count("module_version") ? args.find("module_version")->second : "",
		args.count("level") ? args.find("level")->second : "");
	send_msg(socket, "text/json", value.write_json(), "");
}

void web_server::heartbeat()
{
	if(++nheartbeat_%3600 == 0) {
		std::cerr << "WRITING DATA...\n";
		timeval start_time, end_time;
		gettimeofday(&start_time, NULL);
		variant v = write_stats();
		std::string data = v.write_json(true);

		if(sys::file_exists("stats-5.json")) {
			sys::remove_file("stats-5.json");
		}

		for(int n = 4; n >= 1; --n) {
			if(sys::file_exists(formatter() << "stats-" << n << ".json")) {
				sys::move_file(formatter() << "stats-" << n << ".json",
				               formatter() << "stats-" << (n+1) << ".json");
			}
		}

		sys::write_file("stats-1.json", data);

		gettimeofday(&end_time, NULL);

		const int time_us = (end_time.tv_sec - start_time.tv_sec)*1000000 + (end_time.tv_usec - start_time.tv_usec);
		std::cerr << "WROTE STATS IN " << time_us << "us\n";
	}

	timer_.expires_from_now(boost::posix_time::seconds(1));
	timer_.async_wait(boost::bind(&web_server::heartbeat, this));
}
