#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "asserts.hpp"
#include "json_parser.hpp"
#include "tbs_client.hpp"

namespace tbs {

void client::send_request(const std::string& request, game_logic::map_formula_callable_ptr callable, boost::function<void(std::string)> handler)
{
	handler_ = handler;
	callable_ = callable;

	http_client::send_request("POST /tbs", 
		request, 
		boost::bind(&client::recv_handler, this, _1), 
		boost::bind(&client::error_handler, this, _1), 
		0);
}

void client::recv_handler(const std::string& msg)
{
	if(handler_) {
		callable_->add("message", json::parse(msg, json::JSON_NO_PREPROCESSOR));
		handler_("message_received");
	}
}

void client::error_handler(const std::string& err)
{
	if(handler_) {
		callable_->add("error", json::parse(err, json::JSON_NO_PREPROCESSOR));
		handler_("connection_error");
	}
}

variant client::get_value(const std::string& key) const
{
	return variant();
}

}
