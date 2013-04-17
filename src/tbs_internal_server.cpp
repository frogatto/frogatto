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

#include "asserts.hpp"
#include "foreach.hpp"
#include "json_parser.hpp"
#include "tbs_internal_server.hpp"
#include "variant_utils.hpp"
#include "wml_formula_callable.hpp"

#if defined(_MSC_VER)
#define strtoll _strtoi64
#endif

namespace tbs
{
	namespace 
	{
		internal_server_ptr server_ptr;
	}

	boost::asio::io_service internal_server::io_service_;

	internal_server::internal_server()
		: server_base(io_service_)
	{
	}

	internal_server::~internal_server()
	{
	}

	void internal_server::send_request(const variant& request, 
		int session_id,
		game_logic::map_formula_callable_ptr callable, 
		boost::function<void(const std::string&)> handler)
	{
		ASSERT_LOG(server_ptr != NULL, "Internal server pointer is NULL");
		send_function send_fn = boost::bind(&internal_server::send_msg, server_ptr.get(), _1, session_id, handler, callable);
		server_ptr->write_queue(send_fn, request, session_id);
	}

	void internal_server::send_msg(const variant& resp, 
		int session_id,
		boost::function<void(const std::string&)> handler, 
		game_logic::map_formula_callable_ptr callable)
	{
		if(handler) {
			callable->add("message", resp);
			handler("message_received");
		}
		disconnect(session_id);
	}

	void internal_server::process()
	{
		ASSERT_LOG(server_ptr != NULL, "Internal server pointer is NULL");
		server_ptr->handle_process();
	}

	void internal_server::init()
	{
		server_ptr = internal_server_ptr(new internal_server);
	}

	int internal_server::requests_in_flight(int session_id)
	{
		int result = 0;
		for(std::map<send_function, socket_info, send_function_less>::iterator i = server_ptr->connections_.begin(); i != server_ptr->connections_.end(); ++i) {
			if(i->second.session_id == session_id) {
			}
		}

		return result;
	}

	server_base::socket_info& internal_server::get_socket_info(send_function send_fn)
	{
		return connections_[send_fn];
	}

	void internal_server::disconnect(int session_id)
	{
		if(session_id == -1) {
			return;
		}

		for(std::map<send_function, socket_info, send_function_less>::iterator i = connections_.begin(); i != connections_.end(); ++i) {
			if(i->second.session_id == session_id) {
				connections_.erase(i);
				return;
			}
		}
		ASSERT_LOG(false, "Trying to erase unknown session_id: " << session_id);
	}

	void internal_server::heartbeat_internal(int send_heartbeat, std::map<int, client_info>& clients)
	{
		std::vector<std::pair<send_function, variant> > messages;

		for(std::map<send_function, socket_info, send_function_less>::iterator i = connections_.begin(); i != connections_.end(); ++i) {
			send_function send_fn = i->first;
			socket_info& info = i->second;
			ASSERT_LOG(info.session_id != -1, "UNKNOWN SOCKET");

			client_info& cli_info = clients[info.session_id];
			if(cli_info.msg_queue.empty() == false) {
			std::cerr << "SEND MESSAGE: (((" << cli_info.msg_queue.front() << ")))\n";
				messages.push_back(std::pair<send_function,variant>(send_fn, game_logic::deserialize_doc_with_objects(cli_info.msg_queue.front())));
				cli_info.msg_queue.pop_front();
			} else if(send_heartbeat) {
				if(!cli_info.game) {
					variant_builder v;
					v.add("type", variant("heartbeat"));
					messages.push_back(std::pair<send_function,variant>(send_fn, v.build()));
				} else {
					variant v = create_heartbeat_packet(cli_info);
					messages.push_back(std::pair<send_function,variant>(send_fn, v));
				}
			}
		}

		for(int i = 0; i != messages.size(); ++i) {
			messages[i].first(messages[i].second);
		}
	}

	void internal_server::handle_process()
	{
		send_function send_fn;
		variant request;
		int session_id;
		if(read_queue(&send_fn, &request, &session_id)) {
			server_ptr->handle_message(
				send_fn,
				boost::bind(&internal_server::finish_socket, this, send_fn, _1),
				boost::bind(&internal_server::get_socket_info, server_ptr.get(), send_fn),
				session_id, 
				request);
		}
		io_service_.poll();
		io_service_.reset();
	}


	void internal_server::queue_msg(int session_id, const std::string& msg, bool has_priority)
	{
		if(session_id == -1) {
			return;
		}

		server_base::queue_msg(session_id, msg, has_priority);
	}

	void internal_server::write_queue(send_function send_fn, const variant& v, int session_id)
	{
		msg_queue_.push_back(boost::make_tuple(send_fn,v,session_id));
	}

	bool internal_server::read_queue(send_function* send_fn, variant* v, int *session_id)
	{
		ASSERT_LOG(send_fn != NULL && v != NULL && session_id != NULL,
			"read_queue called with NULL parameter.");
		if(msg_queue_.empty()) {
			return false;
		}
		boost::tie(*send_fn, *v, *session_id) = msg_queue_.front();
		msg_queue_.pop_front();
		return true;
	}

	void internal_server::finish_socket(send_function send_fn, client_info& cli_info)
	{
		if(cli_info.msg_queue.empty() == false) {
			const std::string msg = cli_info.msg_queue.front();
			cli_info.msg_queue.pop_front();
			std::cerr << "SEND MESSAGE: (((" << msg << ")))\n";
			send_fn(game_logic::deserialize_doc_with_objects(msg));
		}
	}
}
