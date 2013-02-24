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
			{
				const game_logic::wml_formula_callable_read_scope read_scope;
				if(resp.has_key(variant("serialized_objects"))) {
					foreach(variant obj_node, resp["serialized_objects"]["character"].as_list()) {
						game_logic::wml_serializable_formula_callable_ptr obj = obj_node.try_convert<game_logic::wml_serializable_formula_callable>();
						ASSERT_LOG(obj.get() != NULL, "ILLEGAL OBJECT FOUND IN SERIALIZATION");
						std::string addr_str = obj->addr();
						const intptr_t addr_id = strtoll(addr_str.c_str(), NULL, 16);

						game_logic::wml_formula_callable_read_scope::register_serialized_object(addr_id, obj);
					}
				}
			}
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
				messages.push_back(std::pair<send_function,variant>(send_fn, json::parse(cli_info.msg_queue.front())));
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
				NULL,
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

		for(std::map<send_function, socket_info, send_function_less>::iterator i = connections_.begin(); i != connections_.end(); ++i) {
			if(i->second.session_id == session_id) {
				i->first(json::parse(msg));
				return;
			}
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
}