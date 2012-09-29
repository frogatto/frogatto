#include <boost/bind.hpp>

#include "asserts.hpp"
#include "formula.hpp"
#include "tbs_bot.hpp"

namespace tbs
{

bot::bot(boost::asio::io_service& service, const std::string& host, const std::string& port, variant v)
  : timer_(service), host_(host), port_(port), script_(v["script"].as_list())
{
	timer_.expires_from_now(boost::posix_time::milliseconds(100));
	timer_.async_wait(boost::bind(&bot::process, this));
}

void bot::process()
{
	if(!client_ && response_.size() < script_.size()) {
		variant script = script_[response_.size()];
		variant send = script["send"];
		if(send.is_string()) {
			send = game_logic::formula(send).execute(*this);
		}

		int session_id = -1;
		if(script.has_key("session_id")) {
			session_id = script["session_id"].as_int();
		}

		ASSERT_LOG(send.is_map(), "NO REQUEST TO SEND: " << send.write_json() << " IN " << script.write_json());
		client_.reset(new client(host_, port_, session_id));
		game_logic::map_formula_callable_ptr callable(new game_logic::map_formula_callable(this));
		client_->send_request(send.write_json(), callable, boost::bind(&bot::handle_response, this, _1, callable));
		std::cerr << "BOT SEND " << send.write_json() << "\n";
	} else if(client_) {
		std::cerr << "BOT PROCESS\n";
		client_->process();
	}

	timer_.expires_from_now(boost::posix_time::milliseconds(100));
	timer_.async_wait(boost::bind(&bot::process, this));
}

void bot::handle_response(const std::string& type, game_logic::formula_callable_ptr callable)
{
	ASSERT_LOG(type != "connection_error", "GOT ERROR BACK WHEN SENDING REQUEST: " << callable->query_value("message").write_json());

	ASSERT_LOG(type == "message_received", "UNRECOGNIZED RESPONSE: " << type);

	std::cerr << "BOT GOT RESPONSE: " << callable->query_value("message").write_json() << "\n";

	variant script = script_[response_.size()];
	if(script.has_key("validate")) {
		variant validate = script["validate"];
		for(int n = 0; n != validate.num_elements(); ++n) {
			game_logic::formula f(validate[n]);
			variant result = f.execute(*callable);
			ASSERT_LOG(result.as_bool(), "VALIDATE FAILED: " << validate[n]);
		}
	}

	response_.push_back(callable->query_value("message"));
	client_.reset();
}

variant bot::get_value(const std::string& key) const
{
	return variant();
}

}
