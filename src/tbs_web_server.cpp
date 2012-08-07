#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <iostream>

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "json_parser.hpp"
#include "string_utils.hpp"
#include "tbs_server.hpp"
#include "tbs_web_server.hpp"
#include "unit_test.hpp"
#include "utils.hpp"
#include "variant.hpp"

namespace tbs {

std::string global_debug_str;

using boost::asio::ip::tcp;

web_server::web_server(server& serv, boost::asio::io_service& io_service, int port)
	: http::web_server(io_service, port), server_(serv)
{
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

void web_server::handle_get(socket_ptr socket, 
	const std::string& url, 
	const std::map<std::string, std::string>& args)
{
	std::cerr << "UNSUPPORTED GET REQUEST" << std::endl;
	disconnect(socket);
}

}

COMMAND_LINE_UTILITY(tbs_server) {
	int port = 23456;
	if(args.size() > 0) {
		std::vector<std::string>::const_iterator it = args.begin();
		while(it != args.end()) {
			if(*it == "--port" || *it == "--listen-port") {
				it++;
				if(it != args.end()) {
					port = atoi(it->c_str());
					ASSERT_LOG(port > 0 && port <= 65535, "tbs_server(): Port must lie in the range 1-65535.");
					it = args.end();
				}
			} else {
				it++;
			}
		}
	}
	std::cerr << "tbs_server(): Listening on port " << std::dec << port << std::endl;
	boost::asio::io_service io_service;
	tbs::server s(io_service);
	tbs::web_server ws(s, io_service, port);
	io_service.run();
}
