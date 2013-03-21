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
#ifndef TBS_WEB_SERVER_HPP_INCLUDED
#define TBS_WEB_SERVER_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "http_server.hpp"
#include "variant.hpp"

namespace tbs {

class server;

class web_server : public http::web_server
{
public:
	static boost::asio::io_service* service();
	static int port();

	//set the debug state that will be sent out as a web page to see what's
	//happening in the server.
	static void set_debug_state(variant v);

	explicit web_server(server& serv, boost::asio::io_service& io_service, int port=23456);
	~web_server();
private:
	web_server(const web_server&);

	virtual void handle_post(socket_ptr socket, variant doc, const http::environment& env);
	virtual void handle_get(socket_ptr socket, const std::string& url, const std::map<std::string, std::string>& args);

	void heartbeat(const boost::system::error_code& error);

	server& server_;
	boost::asio::deadline_timer timer_;
};

}

#endif
