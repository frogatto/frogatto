#ifndef MODULE_SERVER_HPP_INCLUDED
#define MODULE_SERVER_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <map>

#include "http_server.hpp"
#include "variant.hpp"

class module_web_server : public http::web_server
{
public:
	explicit module_web_server(const std::string& data_path, boost::asio::io_service& io_service, int port=23456);
	virtual ~module_web_server()
	{}
private:
	void heartbeat();

	virtual void handle_post(socket_ptr socket, variant doc, const http::environment& env);
	virtual void handle_get(socket_ptr socket, const std::string& url, const std::map<std::string, std::string>& args);

	boost::asio::deadline_timer timer_;
	int nheartbeat_;

	std::string data_path_;
};

#endif
