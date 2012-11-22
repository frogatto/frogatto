#include <boost/bind.hpp>

#include "asserts.hpp"
#include "formula.hpp"
#include "tbs_bot.hpp"
#include "tbs_web_server.hpp"

namespace tbs
{

bot::bot(boost::asio::io_service& service, const std::string& host, const std::string& port, variant v)
  : service_(service), timer_(service), host_(host), port_(port), script_(v["script"].as_list())
{
	std::cerr << "CREATE BOT\n";
	timer_.expires_from_now(boost::posix_time::milliseconds(100));
	timer_.async_wait(boost::bind(&bot::process, this));
}

bot::~bot()
{
	timer_.cancel();
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

		std::cerr << "BOT SEND REQUEST\n";

		ASSERT_LOG(send.is_map(), "NO REQUEST TO SEND: " << send.write_json() << " IN " << script.write_json());
		client_.reset(new client(host_, port_, session_id, &service_));
		game_logic::map_formula_callable_ptr callable(new game_logic::map_formula_callable(this));
		client_->send_request(send.write_json(), callable, boost::bind(&bot::handle_response, this, _1, callable));
	}

	timer_.expires_from_now(boost::posix_time::milliseconds(100));
	timer_.async_wait(boost::bind(&bot::process, this));
}

void bot::handle_response(const std::string& type, game_logic::formula_callable_ptr callable)
{
	ASSERT_LOG(type != "connection_error", "GOT ERROR BACK WHEN SENDING REQUEST: " << callable->query_value("message").write_json());

	ASSERT_LOG(type == "message_received", "UNRECOGNIZED RESPONSE: " << type);

	variant script = script_[response_.size()];
	std::vector<variant> validations;
	if(script.has_key("validate")) {
		variant validate = script["validate"];
		for(int n = 0; n != validate.num_elements(); ++n) {
			std::map<variant,variant> m;
			m[variant("validate")] = validate[n];
			game_logic::formula f(validate[n]);
			variant result = f.execute(*callable);
			m[variant("success")] = variant(result.as_bool());
			validations.push_back(variant(&m));
		}
	}

	std::map<variant,variant> m;
	m[variant("message")] = callable->query_value("message");
	m[variant("validations")] = variant(&validations);

	response_.push_back(variant(&m));
	client_.reset();

	tbs::web_server::set_debug_state(generate_report());
	std::cerr << "SET STATE: " << generate_report().write_json() << "\n";
}

variant bot::get_value(const std::string& key) const
{
	return variant();
}

variant bot::generate_report() const
{
	std::map<variant,variant> m;
	std::vector<variant> response = response_;
	m[variant("response")] = variant(&response);
	return variant(&m);
}

}
