#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <iostream>

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "json_parser.hpp"
#include "module.hpp"
#include "string_utils.hpp"
#include "tbs_bot.hpp"
#include "tbs_server.hpp"
#include "tbs_web_server.hpp"
#include "unit_test.hpp"
#include "utils.hpp"
#include "variant.hpp"

namespace tbs {

namespace {
web_server* web_server_instance = NULL;
}

std::string global_debug_str;

using boost::asio::ip::tcp;

web_server::web_server(server& serv, boost::asio::io_service& io_service, int port)
	: http::web_server(io_service, port), server_(serv), timer_(io_service)
{
	web_server_instance = this;
	timer_.expires_from_now(boost::posix_time::milliseconds(1000));
	timer_.async_wait(boost::bind(&web_server::heartbeat, this));
}

web_server::~web_server()
{
	web_server_instance = NULL;
}

void web_server::handle_post(socket_ptr socket, variant doc, const http::environment& env)
{
	int session_id = -1;
	std::map<std::string, std::string>::const_iterator i = env.find("cookie");
	if(i != env.end()) {
		const char* cookie_start = strstr(i->second.c_str(), " session=");
		if(cookie_start != NULL) {
			++cookie_start;
		} else {
			cookie_start = strstr(i->second.c_str(), "session=");
			if(cookie_start != i->second.c_str()) {
				cookie_start = NULL;
			}
		}

		if(cookie_start) {
			session_id = atoi(cookie_start+8);
		}
	}

	if(doc["debug_session"].is_bool()) {
		session_id = doc["debug_session"].as_bool();
	}

	server_.adopt_ajax_socket(socket, session_id, doc);
	return;
}

namespace {
struct KnownFile {
	const char* url;
	const char* fname;
	const char* type;
};

const KnownFile known_files[] = {
	{"/tbs_monitor.html", "data/tbs/tbs_monitor.html", "text/html"},
	{"/tbs_monitor.js", "data/tbs/tbs_monitor.js", "text/javascript"},
};

variant current_debug_state;
int debug_state_id = 0;
std::string current_debug_state_msg = "{ \"new_data\": false }";

std::vector<socket_ptr> debug_state_sockets;

}

void web_server::set_debug_state(variant v)
{
	debug_state_id = rand();
	current_debug_state = v;
	std::map<variant, variant> m;
	m[variant("info")] = current_debug_state;
	m[variant("state")] = variant(debug_state_id);
	m[variant("new_data")] = variant(true);
	current_debug_state_msg = variant(&m).write_json();
	foreach(socket_ptr sock, debug_state_sockets) {
		web_server_instance->send_msg(sock, "text/json", current_debug_state_msg, "");
	}
	debug_state_sockets.clear();
}

void web_server::heartbeat()
{
	foreach(socket_ptr sock, debug_state_sockets) {
		send_msg(sock, "text/json", "{ \"new_data\": false }", "");
		fprintf(stderr, "send no new data\n");

	}
	debug_state_sockets.clear();
	timer_.expires_from_now(boost::posix_time::milliseconds(1000));
	timer_.async_wait(boost::bind(&web_server::heartbeat, this));
}

void web_server::handle_get(socket_ptr socket, 
	const std::string& url, 
	const std::map<std::string, std::string>& args)
{
	if(url == "/tbs_monitor") {
		std::map<std::string,std::string>::const_iterator state_arg = args.find("state");
		if(state_arg != args.end()) {
			const int state_id = atoi(state_arg->second.c_str());
			if(state_id == debug_state_id) {
				debug_state_sockets.push_back(socket);
				return;
			}

			fprintf(stderr, "send debug msg: %s\n", current_debug_state_msg.c_str());
			send_msg(socket, "text/json", current_debug_state_msg, "");
			return;
		}
	}

	foreach(const KnownFile& f, known_files) {
		if(url == f.url) {
			send_msg(socket, f.type, sys::read_file(f.fname), "");
			return;
		}
	}

	std::cerr << "UNSUPPORTED GET REQUEST" << std::endl;
	disconnect(socket);
}

}

namespace {
struct code_modified_exception {};

void on_code_modified()
{
	fprintf(stderr, "code modified\n");
	tbs::game::reload_game_types();
	throw code_modified_exception();
}
}

COMMAND_LINE_UTILITY(tbs_server) {
	int port = 23456;
	std::vector<std::string> bot_id;
	if(args.size() > 0) {
		std::vector<std::string>::const_iterator it = args.begin();
		while(it != args.end()) {
			if(*it == "--port" || *it == "--listen-port") {
				it++;
				if(it != args.end()) {
					port = atoi(it->c_str());
					ASSERT_LOG(port > 0 && port <= 65535, "tbs_server(): Port must lie in the range 1-65535.");
					++it;
				}
			} else if(*it == "--bot") {
				++it;
				if(it != args.end()) {
					bot_id.push_back(*it++);
				}
			} else {
				++it;
			}
		}
	}

	const std::string MonitorDirs[] = { "data/tbs", "data/tbs_test" };
	foreach(const std::string& dir, MonitorDirs) {
		std::vector<std::string> files;
		module::get_files_in_dir(dir, &files);
		foreach(const std::string& fname, files) {
			if(fname.size() > 4 && std::string(fname.end()-4,fname.end()) == ".cfg") {
				std::string path = module::map_file(dir + "/" + fname);
				std::cerr << "NOTIFY ON: " << path << "\n";
				sys::notify_on_file_modification(path, on_code_modified);
			}
		}
	}

	std::cerr << "MONITOR URL: " << "http://localhost:" << port << "/tbs_monitor.html\n";

	boost::asio::io_service io_service;

	std::cerr << "tbs_server(): Listening on port " << std::dec << port << std::endl;
	tbs::server s(io_service);
	tbs::web_server ws(s, io_service, port);

	std::vector<boost::intrusive_ptr<tbs::bot> > bots;
	for(;;) {
		try {
			const assert_recover_scope assert_scope;
			foreach(const std::string& id, bot_id) {
				bots.push_back(boost::intrusive_ptr<tbs::bot>(new tbs::bot(io_service, "localhost", formatter() << port, json::parse_from_file("data/tbs_test/" + id + ".cfg"))));
			}
		} catch(validation_failure_exception& e) {
			std::map<variant,variant> m;
			m[variant("error")] = variant(e.msg);
			tbs::web_server::set_debug_state(variant(&m));
		} catch(json::parse_error& e) {
			std::map<variant,variant> m;
			m[variant("error")] = variant(e.message);
			tbs::web_server::set_debug_state(variant(&m));
		}
	
		try {
			io_service.run();
		} catch(code_modified_exception&) {
			s.clear_games();
		}
	}
}
