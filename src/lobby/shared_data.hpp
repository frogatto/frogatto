#pragma once
#ifndef SHARED_DATA_HPP_INCLUDED
#define SHARED_DATA_HPP_INCLUDED

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <json_spirit.h>
#include <iostream>
#include <map>
#include <vector>

namespace game_server
{
	struct client_info
	{
		client_info()
		{}
		client_info(int sid, bool human, const std::string& slt)
			: session_id(sid), is_human(human), salt(slt)
		{}
		int session_id;
		std::string salt;
		bool is_human;
	};

	struct server_info
	{
		int min_players;
		int max_players;
		std::string name;
		std::string display_name;
		bool has_bots;
	};

	typedef std::map<std::string, client_info> client_map;

	struct game_info 
	{
		bool started;
		int bot_count;
		std::string name;
		std::vector<std::string> clients;
	};

	typedef std::map<int, game_info> game_list;
	class shared_data
	{
	public:
		enum action 
		{
			send_salt,
			user_not_found,
			password_failed,
			login_success,
			bad_session_id,
		};
		shared_data()
		{}
		virtual ~shared_data()
		{}
		boost::tuple<action, client_info> process_user(const std::string& uname, 
			const std::string& phash, 
			int session_id);
		bool sign_off(const std::string& uname, int session_id);
		void check_add_client(const std::string& user, const client_info& ci);
		void check_add_game(int gid, const game_info& gi);
		void get_status_list(json_spirit::mArray* ary);
		void get_server_info(json_spirit::mArray* ary);
		void add_server(const server_info& si);
	private:
		game_list games_;
		client_map clients_;
		std::vector<server_info> servers_;

		boost::mutex guard_;

		shared_data(shared_data&);
	};
}

#endif
