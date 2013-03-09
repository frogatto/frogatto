#ifdef _MSC_VER
#include "targetver.h"
#endif

#include <map>
#include <set>
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

		struct worker_comparator 
		{
			bool operator() (const boost::shared_ptr<worker>& lhs, const boost::shared_ptr<worker>& rhs) const
			{
				return lhs->server_load() < rhs->server_load();
			}
		};

		typedef std::map<std::string, std::set<boost::shared_ptr<worker>, worker_comparator> > game_registry_map;
		game_registry_map& game_registry()
		{
			static game_registry_map res;
			return res;
		}
	}

	worker::worker(int64_t polling_interval, 
		game_server::shared_data& data, 
		const std::string& addr, 
		const std::string& port)
		: running_(true), data_(data),
		polling_interval_(polling_interval),
		server_address_(addr), server_port_(port),
		server_load_(0.0), q_(message_queue_ptr(new message_queue))
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
		while(running_) {
			if(got_server_info == false) {
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
						int field_counter = 0;
						for(auto it : obj) {
							if(it.first == "name") {
								si_.name = it.second.get_str();
								++field_counter;
							} else if(it.first == "display_name") {
								si_.display_name = it.second.get_str();
								++field_counter;
							} else if(it.first == "min_players") {
								si_.min_players = it.second.get_int();
								++field_counter;
							} else if(it.first == "min_humans") {
								si_.min_humans = it.second.get_int();
								++field_counter;
							} else if(it.first == "max_players") {
								si_.max_players = it.second.get_int();
								++field_counter;
							} else if(it.first == "has_bots") {
								si_.has_bots = it.second.get_bool();
								++field_counter;
							} else {
								si_.other[it.first] = it.second;
							}
						}

						if(field_counter != 6) {
							throw new processing_exception("Missing attribute in get_server_info reply.");
						}

						si_.server_address = server_address_;
						si_.server_port = server_port_;
						got_server_info = true;
						data_.add_server(si_);

						// Register the game
						game_registry()[si_.name].insert(boost::shared_ptr<worker>(this));
					}
				} catch(std::exception& e) {
					std::cerr << "exception: " << e.what() << " " << server_address_ << ":" << server_port_ << std::endl;
				}
				boost::this_thread::sleep(boost::posix_time::milliseconds(polling_interval_));
			} else {
				// got_server_info == true
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
					std::cerr << "exception: " << e.what() << std::endl;
					got_server_info = false;
				}
				message msg;
				if(q_->wait_and_pop(msg, polling_interval_)) {
					http::client::reply reply;
					http::client::request req;
					req.body = msg.msg;
					try {
						if(http::client::client(server_address_, server_port_, req, reply)) {
							msg.reply->push(reply.body);
						} else {
							msg.reply->push("{\"type\":\"error\", \"description\":\"No response from game server.\"");
						}
					} catch(std::exception& e) {
						msg.reply->push("{\"type\":\"error\", \"description\":\"Exceptional response from game server.\"");
						std::cerr << "exception: " << e.what() << std::endl;
						got_server_info = false;
					}
				}
			}
		}
	}

	void worker::abort()
	{
		running_ = false;
	}

	worker& worker::get_server_from_game_type(const std::string& game_type)
	{
		auto gt = game_registry().find(game_type);
		if(gt == game_registry().end()) {
			throw new processing_exception("No server for game: " + game_type);
		}
		return *(*gt->second.begin());
	}

	void worker::get_server_info(json_spirit::mObject* obj)
	{
		for(auto game : game_registry()) {
			const server_info& si = (*game.second.begin())->get_server_info();
			json_spirit::mObject g_obj;
			g_obj["display_name"] = si.display_name;
			g_obj["min_players"] = int(si.min_players);
			g_obj["min_humans"] = int(si.min_humans);
			g_obj["max_players"] = int(si.max_players);
			g_obj["has_bots"] = si.has_bots;
			for(auto it : si.other) {
				g_obj[it.first] = it.second;
			}
			(*obj)[game.first] = g_obj;
		}
	}
}

