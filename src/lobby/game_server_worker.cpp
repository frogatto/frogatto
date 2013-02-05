#ifdef _MSC_VER
#include "targetver.h"
#endif

#include <boost/asio.hpp>
#include <json_spirit.h>

#include "game_server_worker.hpp"
#include "client.hpp"
#include "header.hpp"

namespace game_server
{
	namespace 
	{
		class processing_exception: public std::runtime_error
		{
		public:
			processing_exception(const std::string& arg): runtime_error(arg) {}
		};
	}

	worker::worker(int64_t polling_interval, 
		game_server::shared_data& data, 
		const std::string& addr, 
		const std::string& port)
		: running_(true), data_(data), polling_interval_(polling_interval),
		server_address_(addr), server_port_(port)
		{}

	void worker::process_game(const json_spirit::mObject& obj)
	{
		game_info gi;
		auto type_it = obj.find("type");
		auto gid_it = obj.find("id");
		auto started_it = obj.find("started");
		auto clients_it = obj.find("clients");
		if(type_it == obj.end() 
			|| gid_it == obj.end() 
			|| started_it == obj.end() 
			|| clients_it == obj.end()) {
			throw new processing_exception("Missing required attribute.");
		}
		if(type_it->second.get_str() != "game_info") {
			throw new processing_exception("'type' is not 'game_info'");
		}
		gi.name = si_.name;
		int gid = gid_it->second.get_int();
		gi.started = started_it->second.get_bool();
		gi.bot_count = 0;
		for(auto cid : clients_it->second.get_array()) {
			auto cobj = cid.get_obj();
			auto nick_it = cobj.find("nick");
			auto cid_it = cobj.find("id");
			auto bot_it = cobj.find("bot");
			if(nick_it == cobj.end() || cid_it == cobj.end() || bot_it == cobj.end()) {
				throw new processing_exception("Missing required attribute.");
			}
			const bool is_bot = bot_it->second.get_bool();
			const std::string& user = nick_it->second.get_str();
			data_.check_add_client(user, client_info(cid_it->second.get_int(), !is_bot, ""));
			if(!is_bot) {
				gi.clients.push_back(user);
			} else {
				++gi.bot_count;
			}
		}
		// Don't add games with no clients.
		if(!gi.clients.empty()) {
			data_.check_add_game(gid, gi);
		} else {
			std::cerr << "Not adding game with no clients: " << gid << ", bots: " << gi.bot_count << std::endl;
		}
	}

	void worker::operator()()
	{
		int last_status = 0;
		bool got_server_info = false;
		// First send a message to get the server info
		while(running_ && !got_server_info) {
			try {
				http::client::reply reply;
				http::client::request req;
				json_spirit::mObject req_obj;
				req_obj["type"] = "get_server_info";
				req.body = json_spirit::write(req_obj);
				if(http::client::client(server_address_, server_port_, req, reply)) {
					json_spirit::mValue value;
					json_spirit::read(reply.body, value);
					if(value.type() != json_spirit::obj_type) {
						throw new processing_exception("Couldn't parse response as json: " + reply.body);
					}
					auto& obj = value.get_obj();
					auto name_it = obj.find("name");
					auto dname_it = obj.find("display_name");
					auto minp_it = obj.find("min_players");
					auto maxp_it = obj.find("max_players");
					auto hasbots_it = obj.find("has_bots");
					if(name_it == obj.end() 
						|| dname_it == obj.end()
						|| minp_it == obj.end()
						|| maxp_it == obj.end()
						|| hasbots_it == obj.end()) {
						throw new processing_exception("Missing attribute in get_server_info reply.");
					}

					got_server_info = true;
					si_.name = name_it->second.get_str();
					si_.display_name = dname_it->second.get_str();
					si_.min_players = minp_it->second.get_int();
					si_.max_players = maxp_it->second.get_int();
					si_.has_bots = hasbots_it->second.get_bool();
					data_.add_server(si_);
				}
			} catch(std::exception& e) {
				std::cerr << "exception: " << e.what() << " " << server_address_ << ":" << server_port_ << std::endl;
			}
			boost::this_thread::sleep(boost::posix_time::milliseconds(polling_interval_));
		}
		// Then do main processing
		while(running_) {
			try {
				http::client::reply reply;
				http::client::request req;
				json_spirit::mObject req_obj;
				req_obj["type"] = "get_status";
				req_obj["last_seen"] = last_status;
				req.body = json_spirit::write(req_obj);
				if(http::client::client(server_address_, server_port_, req, reply)) {
					json_spirit::mValue value;
					json_spirit::read(reply.body, value);
					if(value.type() != json_spirit::obj_type) {
						throw new processing_exception("Couldn't parse response as json: " + reply.body);
					}
					auto& obj = value.get_obj();
					auto games_it = obj.find("games");
					auto status_it = obj.find("status_id");
					auto type_it = obj.find("type");
					if(games_it == obj.end()) {
						throw new processing_exception("Returned object must have 'games' attribute");
					}
					if(status_it == obj.end()) {
						throw new processing_exception("Returned object must have 'status_id' attribute");
					}
					if(type_it == obj.end()) {
						throw new processing_exception("Returned object must have 'type' attribute");
					}
					// update our last status code
					last_status = status_it->second.get_int();

					std::cerr << "POLL: " << type_it->second.get_str() << " " << last_status << std::endl;
					if(games_it->second.type() == json_spirit::array_type) {
						for(auto it : games_it->second.get_array()) {
							process_game(it.get_obj());
						}
					} else {
						throw new processing_exception("Type of games");
					}
				}
			} catch(processing_exception& e) {
				std::cerr << "http client exception: " << e.what() << std::endl;
			} catch(std::exception& e) {
				//ASSERT_LOG(false, "exception: " << e.what());
				std::cerr << "exception: " << e.what() << std::endl;
			}
			boost::this_thread::sleep(boost::posix_time::milliseconds(polling_interval_));
		}
	}

	void worker::abort()
	{
		running_ = false;
	}
}

