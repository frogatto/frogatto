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
#pragma once
#ifndef TBS_SERVER_VIRT_HPP_INCLUDED
#define TBS_SERVER_VIRT_HPP_INCLUDED

#include <boost/function.hpp>

#include <map>
#include <vector>

#include "tbs_game.hpp"
#include "variant.hpp"

namespace tbs
{
	typedef boost::function<void(variant)> send_function;

	class server_base
	{
	public:
		server_base(boost::asio::io_service& io_service);
		virtual ~server_base();
		
		void clear_games();
		static variant get_server_info();
	protected:
		struct game_info 
		{
			game_info(const variant& value);
			game_ptr game_state;
			std::vector<int> clients;
			int nlast_touch;
		};

		typedef boost::shared_ptr<game_info> game_info_ptr;
		struct client_info 
		{
			client_info();

			std::string user;	
			game_info_ptr game;
			int nplayer;
			int last_contact;

			int session_id;

			std::deque<std::string> msg_queue;
		};

		struct socket_info 
		{
			socket_info() : session_id(-1) {}
			std::vector<char> partial_message;
			std::string nick;
			int session_id;
		};

		virtual void handle_message(send_function send_fn, 
			boost::function<void(client_info&)> close_fn,
			boost::function<socket_info&(void)> socket_info_fn,
			int session_id, 
			const variant& msg);

		virtual void queue_msg(int session_id, const std::string& msg, bool has_priority=false);

		virtual void heartbeat_internal(int send_heartbeat, std::map<int, client_info>& clients) = 0;

		variant create_heartbeat_packet(const client_info& cli_info);

	private:
		variant create_lobby_msg() const;
		variant create_game_info_msg(game_info_ptr g) const;
		void status_change();
		void quit_games(int session_id);
		void flush_game_messages(game_info& info);
		void schedule_write();
		void handle_message_internal(client_info& cli_info, const variant& msg);
		void heartbeat(const boost::system::error_code& error);

		int nheartbeat_;
		int scheduled_write_;
		int status_id_;

		std::map<int, client_info> clients_;
		std::vector<game_info_ptr> games_;

		boost::asio::deadline_timer timer_;

		// send_fn's waiting on status info.
		std::vector<send_function> status_fns_;
	};
}

#endif
