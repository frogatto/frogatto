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
	//set the debug state that will be sent out as a web page to see what's
	//happening in the server.
	static void set_debug_state(variant v);

	explicit web_server(server& serv, boost::asio::io_service& io_service, int port=23456);
	~web_server();
private:
	web_server(const web_server&);

	virtual void handle_post(socket_ptr socket, variant doc, const http::environment& env);
	virtual void handle_get(socket_ptr socket, const std::string& url, const std::map<std::string, std::string>& args);

	void heartbeat();

	server& server_;
	boost::asio::deadline_timer timer_;
};

}

#endif
