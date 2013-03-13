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

class game;

class client : public http_client
{
public:
	client(const std::string& host, const std::string& port, int session=-1, boost::asio::io_service* service=NULL)
		: http_client(host, port, session, service),
		  use_local_cache_(true),
		  local_nplayer_(-1)
	{}
	void send_request(const variant& request, 
		game_logic::map_formula_callable_ptr callable, 
		boost::function<void(std::string)> handler);

	virtual void process();

	void set_use_local_cache(bool value) { use_local_cache_ = value; }
private:
	boost::function<void(std::string)> handler_;
	game_logic::map_formula_callable_ptr callable_;

	void recv_handler(const std::string& msg);
	void error_handler(const std::string& err);
	variant get_value(const std::string& key) const;

	bool use_local_cache_;
	boost::shared_ptr<tbs::game> local_game_cache_;
	int local_nplayer_;

	std::vector<std::string> local_responses_;
};

}

#endif
