#pragma once
#ifndef TBS_INTERNAL_SERVER_HPP_INCLUDED
#define TBS_INTERNAL_SERVER_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <deque>

#include "formula_callable.hpp"
#include "tbs_server_base.hpp"
#include "variant.hpp"

namespace tbs
{
	struct send_function_less
	{
		bool operator()(const send_function& lhs, const send_function& rhs)
		{
			return (&lhs < &rhs);
		}
	};

	class internal_server : public server_base
	{
	public:
		internal_server();
		virtual ~internal_server();

		void handle_process();

		static void send_request(const variant& request, 
			int session_id,
			game_logic::map_formula_callable_ptr callable, 
			boost::function<void(const std::string&)> handler);		
		static void process();
		static void init();
		static boost::asio::io_service& get_io_service() { return io_service_; }
	protected:
		virtual void heartbeat_internal(int send_heartbeat, std::map<int, client_info>& clients);
	private:
		void send_msg(const variant& resp, 
			int session_id,
			boost::function<void(const std::string&)> handler, 
			game_logic::map_formula_callable_ptr callable);
		static boost::asio::io_service io_service_;

		void write_queue(send_function send_fn, const variant& v, int session_id);
		bool read_queue(send_function* send_fn, variant* v, int *session_id);
		
		socket_info& get_socket_info(send_function send_fn);
		void disconnect(int session_id);
		void queue_msg(int session_id, const std::string& msg, bool has_priority);

		std::map<send_function, socket_info, send_function_less> connections_;
		std::deque<boost::tuple<send_function,variant,int> > msg_queue_;
	};

	typedef boost::shared_ptr<internal_server> internal_server_ptr;
}

#endif