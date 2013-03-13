#include <boost/bind.hpp>

#include "asserts.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "preferences.hpp"
#include "tbs_bot.hpp"
#include "tbs_web_server.hpp"

namespace tbs
{

bot::bot(boost::asio::io_service& service, const std::string& host, const std::string& port, variant v)
  : service_(service), timer_(service), host_(host), port_(port), script_(v["script"].as_list()),
    on_create_(game_logic::formula::create_optional_formula(v["on_create"])),
    on_message_(game_logic::formula::create_optional_formula(v["on_message"]))

{
	std::cerr << "CREATE BOT\n";
	timer_.expires_from_now(boost::posix_time::milliseconds(100));
	timer_.async_wait(boost::bind(&bot::process, this, boost::asio::placeholders::error));
}

bot::~bot()
{
	timer_.cancel();
}

void bot::process(const boost::system::error_code& error)
{
	if(error == boost::asio::error::operation_aborted) {
		std::cerr << "tbs::bot::process cancelled" << std::endl;
		return;
	}
	if(on_create_) {
		execute_command(on_create_->execute(*this));
		on_create_.reset();
	}

	if(((!client_ && !preferences::internal_tbs_server()) || (!internal_client_ && preferences::internal_tbs_server()))
		&& response_.size() < script_.size()) {
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
		game_logic::map_formula_callable_ptr callable(new game_logic::map_formula_callable(this));
		if(preferences::internal_tbs_server()) {
			internal_client_.reset(new internal_client(session_id));
			internal_client_->send_request(send, session_id, callable, boost::bind(&bot::handle_response, this, _1, callable));
		} else {
			client_.reset(new client(host_, port_, session_id, &service_));
			client_->set_use_local_cache(false);
			client_->send_request(send, callable, boost::bind(&bot::handle_response, this, _1, callable));
		}
	}

	timer_.expires_from_now(boost::posix_time::milliseconds(100));
	timer_.async_wait(boost::bind(&bot::process, this, boost::asio::placeholders::error));
}

void bot::handle_response(const std::string& type, game_logic::formula_callable_ptr callable)
{
	std::cerr << "BOT HANDLE RESPONSE...\n";
	if(on_create_) {
		execute_command(on_create_->execute(*this));
		on_create_.reset();
	}

	if(on_message_) {
		std::cerr << "BOT ON MESSAGE\n";
		message_type_ = type;
		message_callable_ = callable;
		execute_command(on_message_->execute(*this));
	}

	ASSERT_LOG(type != "connection_error", "GOT ERROR BACK WHEN SENDING REQUEST: " << callable->query_value("message").write_json());

	ASSERT_LOG(type == "message_received", "UNRECOGNIZED RESPONSE: " << type);

	variant script = script_[response_.size()];
	std::vector<variant> validations;
	if(script.has_key("validate")) {
		variant validate = script["validate"];
		for(int n = 0; n != validate.num_elements(); ++n) {
			std::map<variant,variant> m;
			m[variant("validate")] = validate[n][variant("expression")] + variant(" EQUALS ") + validate[n][variant("equals")];

			game_logic::formula f(validate[n][variant("expression")]);
			variant expression_result = f.execute(*callable);

			if(expression_result != validate[n][variant("equals")]) {
				m[variant("error")] = variant(1);
			}
			m[variant("value")] = expression_result;

			validations.push_back(variant(&m));
		}
	}

	std::map<variant,variant> m;
	m[variant("message")] = callable->query_value("message");
	m[variant("validations")] = variant(&validations);

	response_.push_back(variant(&m));
	client_.reset();
	internal_client_.reset();

	tbs::web_server::set_debug_state(generate_report());
}

variant bot::get_value(const std::string& key) const
{
	if(key == "script") {
		std::vector<variant> s = script_;
		return variant(&s);
	} else if(key == "data") {
		return data_;
	} else if(key == "type") {
		return variant(message_type_);
	} else if(key == "me") {
		return variant(this);
	} else if(message_callable_) {
		return message_callable_->query_value(key);
	}
	return variant();
}

void bot::set_value(const std::string& key, const variant& value)
{
	if(key == "script") {
		std::cerr << "BOT SET SCRIPT\n";
		script_ = value.as_list();
	} else if(key == "data") {
		data_ = value;
	} else {
		ASSERT_LOG(false, "ILLEGAL VALUE SET IN TBS BOT: " << key);
	}
}

variant bot::generate_report() const
{
	std::map<variant,variant> m;
	std::vector<variant> response = response_;
	m[variant("response")] = variant(&response);
	return variant(&m);
}

}
