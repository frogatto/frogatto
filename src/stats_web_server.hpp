#ifndef STATS_WEB_SERVER_HPP_INCLUDED
#define STATS_WEB_SERVER_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "http_server.hpp"

class web_server : public http::web_server
{
public:
	explicit web_server(boost::asio::io_service& io_service, int port=23456);
private:
	void heartbeat();

	virtual void handle_post(socket_ptr socket, variant doc, const http::environment& env);
	virtual void handle_get(socket_ptr socket, const std::string& url, const std::map<std::string, std::string>& args);

	boost::asio::deadline_timer timer_;
	int nheartbeat_;
};

#endif
