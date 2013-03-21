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
#ifndef HTTP_SERVER_HPP_INCLUDED
#define HTTP_SERVER_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <map>

namespace http {

typedef std::map<std::string, std::string> environment;

class web_server
{
public:
	typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
	typedef boost::shared_ptr<boost::array<char, 64*1024> > buffer_ptr;

	explicit web_server(boost::asio::io_service& io_service, int port=23456);
	virtual ~web_server();

	static void disconnect_socket(socket_ptr socket);

protected:
	void start_accept();
	void handle_accept(socket_ptr socket, const boost::system::error_code& error);

	void send_msg(socket_ptr socket, const std::string& type, const std::string& msg, const std::string& header_parms);
	void send_404(socket_ptr socket);

	void handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, size_t max_bytes, boost::shared_ptr<std::string> buf);

	virtual void disconnect(socket_ptr socket);

	virtual void handle_post(socket_ptr socket, variant doc, const environment& env) = 0;
	virtual void handle_get(socket_ptr socket, const std::string& url, const std::map<std::string, std::string>& args) = 0;

private:
	struct receive_buf {
		receive_buf() : wanted(0) {}
		std::string msg;
		size_t wanted;
	};
	typedef boost::shared_ptr<receive_buf> receive_buf_ptr;

	void start_receive(socket_ptr socket, receive_buf_ptr buf=receive_buf_ptr());
	void handle_receive(socket_ptr socket, buffer_ptr buf, const boost::system::error_code& e, size_t nbytes, receive_buf_ptr recv_buf);
	void handle_incoming_data(socket_ptr socket, const char* i1, const char* i2, receive_buf_ptr recv_buf);

	void handle_message(socket_ptr socket, receive_buf_ptr recv_buf);

	boost::asio::ip::tcp::acceptor acceptor_;
};

}

#endif
