#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/uuid/sha1.hpp>

#include "shared_data.hpp"

namespace game_server 
{
	namespace
	{
		const std::string fixed_password = "Hello there";

		bool check_password(const std::string& salt, const std::string& pword, const std::string& phash)
		{
			boost::uuids::detail::sha1 s;
			std::string mash = salt + pword;
			s.process_bytes(mash.c_str(), mash.length());
			unsigned int digest[5];
			s.get_digest(digest);
			std::stringstream str;
			str << std::hex << std::setfill('0')  << std::setw(sizeof(unsigned int)*2) << digest[0] << digest[1] << digest[2] << digest[3] << digest[4];
			return (phash.compare(str.str()) == 0);
		}

		// Global generator, so it's not being constantly instantiated.
		boost::mt19937 gen;

		int generate_session_id()
		{
			boost::uniform_int<> dist(1, 2147483647);
			boost::variate_generator<boost::mt19937&, boost::uniform_int<> > sid(gen, dist);
			sid.engine().seed(static_cast<uint32_t>(time(nullptr)));
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
		boost::mutex::scoped_lock lock(guard_);
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
			return boost::make_tuple(send_salt, it->second);
		} else {
			if(check_password(it->second.salt, fixed_password, phash)) {
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

	void shared_data::get_status_list(json_spirit::mObject* users)
	{
		boost::mutex::scoped_lock lock(guard_);
		for(auto u : clients_) {
			json_spirit::mObject uo;
			const std::string& user = u.first;
			uo["waiting_for_players"] = u.second.waiting_for_players;
			uo["created_game"] = u.second.game;
			auto it = std::find_if(games_.begin(), games_.end(), 
				[user](const std::pair<int, game_info>& v) { return std::find(v.second.clients.begin(), v.second.clients.end(), user) != v.second.clients.end(); });
			uo["game"] = it == games_.end() ? "lobby" : it->second.name;

			(*users)[user] = uo;
		}
	}

	void shared_data::check_add_client(const std::string& user, const client_info& ci)
	{
		boost::mutex::scoped_lock lock(guard_);
		if(ci.is_human) {
			auto it = clients_.find(user);
			if(it == clients_.end()) {
				// user not on list add it -- recovery from lobby being killed/breaking.
				clients_[user] = ci;
			} else {
				// user on list check compare details.
				if(it->second.session_id != ci.session_id) {
					std::cerr << "Detected user with multiple session ID's, correcting: " << it->second.session_id << ":" << ci.session_id << std::endl;
					clients_[user] = ci;
				}
			}
		}
	}

	void shared_data::check_add_game(int gid, const game_info& gi)
	{
		boost::mutex::scoped_lock lock(guard_);
		auto it = games_.find(gid);
		if(it == games_.end()) {
			// Game not on list!
			games_[gid] = gi;
		}
	}

	void shared_data::add_server(const server_info& si)
	{
		boost::mutex::scoped_lock lock(guard_);
		servers_.push_back(si);
	}

	bool shared_data::create_game(const std::string& user, const std::string& game_type)
	{
		boost::mutex::scoped_lock lock(guard_);
		auto it = clients_.find(user);
		if(it == clients_.end()) {
			return false;
		}
		it->second.game	= game_type;
		it->second.waiting_for_players = true;
		return true;
	}

	int shared_data::make_session_id()
	{
		return generate_session_id();
	}
}