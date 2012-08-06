#ifndef TBS_CLIENT_HPP_INCLUDED
#define TBS_CLIENT_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

#include "formula_callable.hpp"
#include "http_client.hpp"

namespace tbs {
using boost::asio::ip::tcp;

class client : public http_client
{
public:
	client(const std::string& host, const std::string& port, int session=-1)
		: http_client(host, port, session)
	{}
	void send_request(const std::string& request, 
		game_logic::map_formula_callable_ptr callable, 
		boost::function<void(std::string)> handler);
private:
	boost::function<void(std::string)> handler_;
	game_logic::map_formula_callable_ptr callable_;

	void recv_handler(const std::string& msg);
	void error_handler(const std::string& err);
	variant get_value(const std::string& key) const;
};

}

#endif
