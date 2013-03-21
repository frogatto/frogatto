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
#ifndef SERVER_HPP_INCLUDED
#define SERVER_HPP_INCLUDED

#include "tbs_game.hpp"
#include "variant.hpp"

#include <deque>
#include <map>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "tbs_server_base.hpp"

namespace tbs {

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
typedef boost::shared_ptr<boost::array<char, 1024> > buffer_ptr;

class server : public server_base
{
public:
	explicit server(boost::asio::io_service& io_service);
	virtual ~server();
	 
	void adopt_ajax_socket(socket_ptr socket, int session_id, const variant& msg);

	static variant get_server_info();
private:
	void close_ajax(socket_ptr socket, client_info& cli_info);

	void send_msg(socket_ptr socket, const variant& msg);
	void send_msg(socket_ptr socket, const char* msg);
	void send_msg(socket_ptr socket, const std::string& msg);
	void handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, boost::shared_ptr<std::string> buf, int session_id);
	virtual void heartbeat_internal(int send_heartbeat, std::map<int, client_info>& clients);

	socket_info& get_socket_info(socket_ptr socket);

	void disconnect(socket_ptr socket);

	virtual void queue_msg(int session_id, const std::string& msg, bool has_priority=false);

	std::map<int, socket_ptr> sessions_to_waiting_connections_;
	std::map<socket_ptr, std::string> waiting_connections_;

	std::map<socket_ptr, socket_info> connections_;
};

}

#endif
