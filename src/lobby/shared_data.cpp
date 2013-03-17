#ifdef _MSC_VER
#include "targetver.h"
#endif

#include <boost/foreach.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/uuid/sha1.hpp>

#include "asserts.hpp"
#include "connection.hpp"
#include "reply.hpp"
#include "shared_data.hpp"

namespace game_server 
{
	namespace
	{
		const std::string fixed_password = "Hello there";

		std::string sha1(const std::string s)
		{
			boost::uuids::detail::sha1 hash;
			hash.process_bytes(s.c_str(), s.length());
			unsigned int digest[5];
			hash.get_digest(digest);
			std::stringstream str;
			str << std::hex << std::setfill('0')  << std::setw(sizeof(unsigned int)*2) << digest[0] << digest[1] << digest[2] << digest[3] << digest[4];
			return str.str();
		}

		bool check_password(const std::string& salt, const std::string& pword, const std::string& phash)
		{
			return (phash.compare(sha1(salt + pword)) == 0);
		}

		// Global generator, so it's not being constantly instantiated.
		boost::mt19937 gen;

		int generate_session_id()
		{
			boost::uniform_int<> dist(1, 2147483647);
			boost::variate_generator<boost::mt19937&, boost::uniform_int<> > sid(gen, dist);
#ifdef BOOST_NO_CXX11_NULLPTR
			sid.engine().seed(static_cast<uint32_t>(time(NULL)));
#else
			sid.engine().seed(static_cast<uint32_t>(time(nullptr)));
#endif
			return sid();
		}

		std::string generate_salt()
		{
			boost::uniform_int<> dist(1, 2147483647);
			boost::variate_generator<boost::mt19937&, boost::uniform_int<> > salt(gen, dist);
			std::string ssalt;
			for(int i = 0; i < 4; i++) {
				std::stringstream str;
				str << std::hex << salt();
				ssalt += str.str();
			}
			return ssalt;
		}
	}

	boost::tuple<shared_data::action, client_info> shared_data::process_user(const std::string& uname, 
		const std::string& phash, 
		int session_id)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		//ASSERT_LOG(db_ptr_ != NULL, "No open database");
		//sqlite3_exec(db_ptr, 
		//	"SELECT password FROM users_table WHERE username = ?", 
		// XXX Get password from database.
		// if(lookup_username_in_database_fails) {
		//     return boost::make_tuple(user_not_found, it->second);
		// }
		auto it = clients_.find(uname);
		if(session_id == -1) {
			// No session id, check if user name in list already.
			if(it == clients_.end()) {
				// User name not in list. Let's add it.
				auto ret = clients_.insert(std::pair<std::string, client_info>(uname, client_info(generate_session_id(), true, generate_salt())));
				it = ret.first;
			}
		} else {
			if(it == clients_.end()) {
				// user not in list, but we've got a session id. Expire session and generate a new id.
				auto ret = clients_.insert(std::pair<std::string, client_info>(uname, client_info(generate_session_id(), true, generate_salt())));
				it = ret.first;
			} else {
				// We have been sent a session_id, check if it's valid.
				if(it->second.session_id != session_id) {
					it->second.signed_in = false;
					return boost::make_tuple(bad_session_id, it->second);
				}
			}
		}
		if(phash.empty()) {
			it->second.session_id = generate_session_id();
			return boost::make_tuple(send_salt, it->second);
		} else {
			if(check_password(it->second.salt, sha1(fixed_password), phash)) {
				it->second.signed_in = true;
				return boost::make_tuple(login_success, it->second);
			} else {
				it->second.signed_in = false;
				return boost::make_tuple(password_failed, it->second);
			}
		}
	}

	bool shared_data::sign_off(const std::string& uname, int session_id) 
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		auto it = clients_.find(uname);
		if(it == clients_.end()) {
			return false;
		}
		if(it->second.session_id != session_id) {
			return false;
		}
		clients_.erase(it);
		return true;
	}

	bool shared_data::check_user_and_session(const std::string& uname, int session_id)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		auto it = clients_.find(uname);
		if(it == clients_.end()) {
			return false;
		}
		if(it->second.session_id != session_id) {
			return false;
		}
		return true;
	}

	const game_info* shared_data::get_game_info(int game_id) const
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		auto game = games_.find(game_id);
		if(game == games_.end()) {
#ifdef BOOST_NO_CXX11_NULLPTR
			return NULL;
#else
			return nullptr;
#endif
		}
		return &game->second;
	}

	bool shared_data::is_user_in_game(const std::string& user, int game_id) const
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		auto game = games_.find(game_id);
		if(game == games_.end()) {
			return false;
		}
		return std::find(game->second.clients.begin(), game->second.clients.end(), user) != game->second.clients.end();
	}

	bool shared_data::is_user_in_any_games(const std::string& user, int* game_id) const
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		for(auto it = games_.begin(); it != games_.end(); ++it) {
			if(std::find(it->second.clients.begin(), it->second.clients.end(), user) != it->second.clients.end()) {
				if(game_id) {
					*game_id = it->first;
				}
				return true;
			}
		}
		return false;
	}

	bool shared_data::check_client_in_games(const std::string& user, int* game_id)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		bool erased_game = false;
		for(auto it = games_.begin(); it != games_.end();) {
			int gid = it->first;
			// Erase user from client list.
			it->second.clients.erase(std::remove(it->second.clients.begin(), it->second.clients.end(), user), it->second.clients.end());
			if(game_id) {
				*game_id = gid;
			}
			// remove games with no users left.
			if(it->second.clients.empty()) {
				games_.erase(it++);
				erased_game = true;

				// Post removed game message to user
				json_spirit::mObject obj;
				obj["type"] = "lobby_remove_game";
				obj["game_id"] = gid;
				post_message_to_client(user, obj);
			} else {
				json_spirit::mObject obj;
				obj["type"] = "lobby_player_left_game";
				obj["user"] = user;
				post_message_to_game_clients(gid, json_spirit::mValue(obj));

				++it;
			}
		}
		return erased_game;
	}

	void shared_data::get_user_list(json_spirit::mArray* users)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
#ifdef BOOST_NO_CXX11_NULLPTR
		ASSERT_LOG(users != NULL, "get_user_list: null pointer passed in");
#else
		ASSERT_LOG(users != nullptr, "get_user_list: null pointer passed in");
#endif
#ifdef BOOST_NO_CXX11_RANGE_BASED_FOR
		BOOST_FOREACH(auto u, clients_) {
#else
		for(auto u : clients_) {
#endif
			users->push_back(u.first);
		}
	}

	void shared_data::get_games_list(json_spirit::mArray* games)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
#ifdef BOOST_NO_CXX11_NULLPTR
		ASSERT_LOG(games != NULL, "get_games_list: null pointer passed in");
#else
		ASSERT_LOG(games != nullptr, "get_games_list: null pointer passed in");
#endif
#ifdef BOOST_NO_CXX11_RANGE_BASED_FOR
		BOOST_FOREACH(auto g, games_) {
#else
		for(auto g : games_) {
#endif
			json_spirit::mObject obj;
			obj["game_type"] = g.second.name;
			obj["game_id"] = g.first;
			obj["max_players"] = int(g.second.max_players);
			json_spirit::mArray user_ary;
#ifdef BOOST_NO_CXX11_RANGE_BASED_FOR
			BOOST_FOREACH(auto client, g.second.clients) {
#else
			for(auto client : g.second.clients) {
#endif
				user_ary.push_back(client);
			}
			obj["users"] = user_ary;
			games->push_back(obj);
		}
	}

	void shared_data::check_add_client(const std::string& user, client_info& ci)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		if(ci.is_human) {
			auto it = clients_.find(user);
			if(it == clients_.end()) {
				// user not on list add it -- recovery from lobby being killed/breaking.
				if(ci.salt.empty()) {
					ci.salt = generate_salt();
				}
				clients_[user] = ci;
			} else {
				// user on list check compare details.
				if(it->second.session_id != ci.session_id && ci.session_id != -1) {
					std::cerr << "Detected user with multiple session ID's, correcting: " << it->second.session_id << ":" << ci.session_id << std::endl;
					if(ci.salt.empty()) {
						ci.salt = generate_salt();
					}
					clients_[user] = ci;
				}
			}
		}
	}

	void shared_data::check_add_game(int gid, const game_info& gi)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		auto it = server_games_.find(gid);
		if(it == server_games_.end()) {
			// Game not on list!
			server_games_[gid] = gi;
		}
	}

	void shared_data::add_server(const server_info& si)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		servers_.push_back(si);
	}

	int shared_data::get_user_session_id(const std::string& user) const
	{
		auto it = clients_.find(user);
		if(it == clients_.end()) {
			return -1;
		}
		return it->second.session_id;
	}

	bool shared_data::create_game(const std::string& user, const std::string& game_type, size_t max_players, int* game_id)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		auto it = clients_.find(user);
		if(it == clients_.end()) {
			return false;
		}
		// user request to create game, delete user from any other games.
		int gid;
		if(is_user_in_any_games(user, &gid)) {
			check_client_in_games(user, &gid);
		}

#ifdef BOOST_NO_CXX11_NULLPTR
		ASSERT_LOG(game_id != NULL, "Invalid game_id pointer passed in");
#else
		ASSERT_LOG(game_id != nullptr, "Invalid game_id pointer passed in");
#endif
		*game_id = make_session_id();
		game_info gi;
		gi.started = false;
		gi.bot_count = 0;
		gi.clients.push_back(user);
		gi.name = game_type;
		gi.max_players = max_players;
		games_[*game_id] = gi;
		return true;
	}

	void shared_data::remove_game(const std::string& user, int game_id)
	{
		auto it = games_.find(game_id);
		if(it != games_.end()) {
			auto client = std::find(it->second.clients.begin(), it->second.clients.end(), user);
			if(client != it->second.clients.end()) {
				games_.erase(it);

				json_spirit::mObject obj;
				obj["type"] = "lobby_remove_game";
				obj["game_id"] = game_id;
				post_message_to_all_clients(obj);
			}
		}
	}

	int shared_data::make_session_id()
	{
		return generate_session_id();
	}

	client_message_queue_ptr shared_data::get_message_queue(const std::string& user)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		auto it = clients_.find(user);
		if(it == clients_.end()) {
			return client_message_queue_ptr();
		}
		return it->second.msg_q;
	}

	void shared_data::set_waiting_connection(const std::string& user, http::server::connection_ptr conn)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		auto it = clients_.find(user);
		if(it == clients_.end()) {
			return;
		}
#ifdef BOOST_NO_CXX11_NULLPTR
		if(it->second.conn == NULL) {
#else
		if(it->second.conn == nullptr) {
#endif
			it->second.conn = conn;
			it->second.counter = 60;
		}
	}

	void shared_data::process_waiting_connections()
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		static int tick_time = 10;

		for(auto it = clients_.begin(); it != clients_.end(); ++it) {
			client_info& ci = it->second;
			json_spirit::mValue mv;
			if(ci.conn && ci.msg_q && ci.msg_q->try_pop(mv)) {
				http::server::reply::create_json_reply(mv, ci.conn->get_reply());
				ci.conn->handle_delayed_write();
				ci.conn.reset();
				ci.counter = 0;
			}
		}

		if(--tick_time == 0) {
			tick_time = 10;

			for(auto it = clients_.begin(); it != clients_.end();) {
				client_info& ci = it->second;
				std::string user;
				if(ci.last_seen_count && --ci.last_seen_count == 0) {
					user = it->first;
					clients_.erase(it++);
				} else {
					++it;
				}
				// remove user from any games
				if(user.empty() == false) {
					check_client_in_games(user);
				}
			}

			for(auto it = clients_.begin(); it != clients_.end(); ++it) {
				client_info& ci = it->second;
				if (it->second.counter && --it->second.counter == 0) {
					if(it->second.conn) {
						json_spirit::mObject obj;
						obj["type"] = "lobby_heartbeat_reply";					
						http::server::reply::create_json_reply(json_spirit::mValue(obj), ci.conn->get_reply());
						ci.conn->handle_delayed_write();
						ci.conn.reset();
					}
				}
			}
		}
	}

	void shared_data::update_last_seen_count(const std::string& user)
	{
		auto it = clients_.find(user);
		if(it == clients_.end()) {
			return;
		}
		it->second.last_seen_count = last_seen_counter_reload_value;
	}

	bool shared_data::post_message_to_client(const std::string& user, const json_spirit::mValue& val)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		auto it = clients_.find(user);
		if(it == clients_.end()) {
			return false;
		}
		if(it->second.msg_q) {
			it->second.msg_q->push(val);
		} else {
			return false;
		}
		return true;
	}

	void shared_data::post_message_to_all_clients(const json_spirit::mValue& val)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		for(auto it = clients_.begin(); it != clients_.end(); ++it) {
			client_info& ci = it->second;
			if(ci.msg_q) {
				ci.msg_q->push(val);
			}
		}
	}

	bool shared_data::post_message_to_game_clients(int game_id, const json_spirit::mValue& val)
	{
		boost::recursive_mutex::scoped_lock lock(guard_);
		auto it = games_.find(game_id);
		if(it == games_.end()) {
			return false;
		}
#ifdef BOOST_NO_CXX11_RANGE_BASED_FOR
		BOOST_FOREACH(auto client, it->second.clients) {
#else
		for(auto client : it->second.clients) {
#endif
			auto cit = clients_.find(client);
			if(cit != clients_.end()) {
				client_info& ci = cit->second;
				if(ci.msg_q) {
					ci.msg_q->push(val);
				}
			}
		}
		return true;
	}

	bool shared_data::check_game_and_client(int game_id, const std::string& user, const std::string& user_to_add)
	{
		auto it = games_.find(game_id);
		if(it == games_.end()) {
			return false;
		}
		auto cit = std::find(it->second.clients.begin(), it->second.clients.end(), user);
		if(cit == it->second.clients.end()) {
			return false;
		}
		if(user_to_add.empty() == false) {
			it->second.clients.push_back(user_to_add);
			//auto cit = clients_.find(user_to_add);
			//if(cit != clients_.end()) {
			//	client_info& ci = cit->second;
			//}
		}
		return true;
	}
}
