/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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

client::client(const std::string& host, const std::string& port,
               int session, boost::asio::io_service* service)
  : http_client(host, port, session, service), use_local_cache_(true),
    local_game_cache_(NULL), local_nplayer_(-1)
{}

void client::send_request(variant request, game_logic::map_formula_callable_ptr callable, boost::function<void(std::string)> handler)
{
	handler_ = handler;
	callable_ = callable;

	std::string request_str = game_logic::serialize_doc_with_objects(request);
	fprintf(stderr, "SEND ((%s))\n", request_str.c_str());

	http_client::send_request("POST /tbs", 
		request_str,
		boost::bind(&client::recv_handler, this, _1), 
		boost::bind(&client::error_handler, this, _1), 
		0);

	if(local_game_cache_ && request["type"].as_string() == "moves" && request["state_id"].as_int() == local_game_cache_->state_id()) {

		variant request_clone = game_logic::deserialize_doc_with_objects(request_str);

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
		variant v = game_logic::deserialize_doc_with_objects(msg);

		if(use_local_cache_ && v["type"].as_string() == "game") {
			local_game_cache_ = new tbs::game(v["game_type"].as_string(), v);
			local_game_cache_holder_.reset(local_game_cache_);

			local_nplayer_= v["nplayer"].as_int();
			std::cerr << "LOCAL: UPDATE CACHE: " << local_game_cache_->state_id() << "\n";
			v = game_logic::deserialize_doc_with_objects(msg);
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
