#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "asserts.hpp"
#include "foreach.hpp"
#include "json_parser.hpp"
#include "tbs_client.hpp"
#include "tbs_game.hpp"
#include "wml_formula_callable.hpp"

#if defined(_MSC_VER)
#define strtoll _strtoi64
#endif

namespace tbs {

namespace {
variant deserialize_doc(const std::string& msg)
{
	variant v;
	{
		const game_logic::wml_formula_callable_read_scope read_scope;
		try {
			v = json::parse(msg);
		} catch(json::parse_error& e) {
			ASSERT_LOG(false, "ERROR PROCESSING MESSAGE RETURNED FROM TBS SERVER DOC: --BEGIN--" << msg << "--END-- ERROR: " << e.error_message());
		}
		if(v.has_key(variant("serialized_objects"))) {
			foreach(variant obj_node, v["serialized_objects"]["character"].as_list()) {
				game_logic::wml_serializable_formula_callable_ptr obj = obj_node.try_convert<game_logic::wml_serializable_formula_callable>();
				ASSERT_LOG(obj.get() != NULL, "ILLEGAL OBJECT FOUND IN SERIALIZATION");
				std::string addr_str = obj->addr();
				const intptr_t addr_id = strtoll(addr_str.c_str(), NULL, 16);

				game_logic::wml_formula_callable_read_scope::register_serialized_object(addr_id, obj);
			}
		}
	}

	return v;
}
}

void client::send_request(const variant& request, game_logic::map_formula_callable_ptr callable, boost::function<void(std::string)> handler)
{
	handler_ = handler;
	callable_ = callable;

	std::string request_str = request.write_json();

	http_client::send_request("POST /tbs", 
		request_str,
		boost::bind(&client::recv_handler, this, _1), 
		boost::bind(&client::error_handler, this, _1), 
		0);

	if(local_game_cache_ && request["type"].as_string() == "moves" && request["state_id"].as_int() == local_game_cache_->state_id()) {

		variant request_clone = deserialize_doc(request_str);

		local_game_cache_->handle_message(local_nplayer_, request_clone);
		std::vector<game::message> messages;
		local_game_cache_->swap_outgoing_messages(messages);
		foreach(const game::message& msg, messages) {
			std::cerr << "LOCAL: RECIPIENTS: ";
			for(int i = 0; i != msg.recipients.size(); ++i) {
				std::cerr << msg.recipients[i] << " ";
			}
			std::cerr << "\n";
			if(std::count(msg.recipients.begin(), msg.recipients.end(), local_nplayer_)) {
				local_responses_.push_back(msg.contents);
			}
		}
		std::cerr << "LOCAL: HANDLE MESSAGE LOCALLY: " << local_responses_.size() << "/" << messages.size() << "\n";
	}
}

void client::recv_handler(const std::string& msg)
{
	if(handler_) {
		variant v = deserialize_doc(msg);

		if(use_local_cache_ && v["type"].as_string() == "game") {
			local_game_cache_.reset(new tbs::game(v["game_type"].as_string(), v));
			local_nplayer_= v["nplayer"].as_int();
			std::cerr << "LOCAL: UPDATE CACHE: " << local_game_cache_->state_id() << "\n";
			v = deserialize_doc(msg);
		}

		//fprintf(stderr, "RECV: (((%s)))\n", msg.c_str());
		//fprintf(stderr, "SERIALIZE: (((%s)))\n", v.write_json().c_str());
		callable_->add("message", v);

//		try {
			handler_("message_received");
//		} catch(...) {
//			std::cerr << "ERROR PROCESSING TBS MESSAGE\n";
//			throw;
//		}
	}
}

void client::error_handler(const std::string& err)
{
	std::cerr << "ERROR IN TBS CLIENT: " << err << (handler_ ? " SENDING TO HANDLER...\n" : " NO HANDLER\n");
	if(handler_) {
		variant v;
		try {
			v = json::parse(err, json::JSON_NO_PREPROCESSOR);
		} catch(const json::parse_error&) {
			std::cerr << "Uanble to parse message \"" << err << "\" assuming it is a string." << std::endl;
		}
		callable_->add("error", v.is_null() ? variant(err) : v);
		handler_("connection_error");
	}
}

variant client::get_value(const std::string& key) const
{
	return http_client::get_value(key);
}

void client::process()
{
	std::vector<std::string> local_responses;
	local_responses.swap(local_responses_);
	foreach(const std::string& response, local_responses) {
		std::cerr << "LOCAL: PROCESS LOCAL RESPONSE: " << response.size() << "\n";
		recv_handler(response);
	}

	http_client::process();
}

}
