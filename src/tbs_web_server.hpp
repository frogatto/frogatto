#ifndef WEB_SERVER_HPP_INCLUDED
#define WEB_SERVER_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "http_server.hpp"

namespace tbs {

class server;

class web_server : public http::web_server
{
public:
	explicit web_server(server& serv, boost::asio::io_service& io_service, int port=23456);
private:

	virtual void handle_post(socket_ptr socket, variant doc, const http::environment& env);
	virtual void handle_get(socket_ptr socket, const std::string& url, const std::map<std::string, std::string>& args);

	server& server_;
};

}

#endif
