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
#include <boost/asio.hpp>

#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "json_parser.hpp"
#include "tbs_server_base.hpp"
#include "variant_utils.hpp"

namespace tbs
{
	namespace 
	{
		const variant& get_server_info_file()
		{
			static variant server_info;
			if(server_info.is_null()) {
				server_info = json::parse_from_file("data/server_info.cfg");
				server_info.add_attr(variant("type"), variant("server_info"));
			}
			return server_info;
		}
	}

	server_base::server_base(boost::asio::io_service& io_service)
		: timer_(io_service), nheartbeat_(0), scheduled_write_(0), status_id_(0)
	{
		heartbeat(boost::asio::error::timed_out);
	}

	server_base::~server_base()
	{
	}

	variant server_base::get_server_info()
	{
		return get_server_info_file();
	}

	void server_base::clear_games()
	{
		games_.clear();
		clients_.clear();
	}

	void server_base::handle_message(send_function send_fn, 
		boost::function<void(client_info&)> close_fn, 
		boost::function<socket_info&(void)> socket_info_fn,
		int session_id, 
		const variant& msg)
	{
		const std::string& type = msg["type"].as_string();

		if(session_id == -1) {
			if(type == "create_game") {
				game_info_ptr g(new game_info(msg));
				if(!g->game_state) {
					std::cerr << "COULD NOT CREATE GAME TYPE: " << msg["game_type"].as_string() << "\n";
					send_fn(json::parse("{ \"type\": \"create_game_failed\" }"));
					return;
				}

				std::vector<variant> users = msg["users"].as_list();
				for(int i = 0; i != users.size(); ++i) {
					const std::string user = users[i]["user"].as_string();
					const int session_id = users[i]["session_id"].as_int();

					if(clients_.count(session_id) && session_id != -1) {
						std::cerr << "ERROR: REUSED SESSION ID WHEN CREATING GAME: " << session_id << "\n";
						send_fn(json::parse("{ \"type\": \"create_game_failed\" }"));
						return;
					}

					client_info& cli_info = clients_[session_id];
					cli_info.user = user;
					cli_info.game = g;
					cli_info.nplayer = i;
					cli_info.last_contact = nheartbeat_;
					cli_info.session_id = session_id;

					if(users[i]["bot"].as_bool(false) == false) {
						g->game_state->add_player(user);
					} else {
						g->game_state->add_ai_player(user, users[i]);
					}

					g->clients.push_back(session_id);
				}

				const game_context context(g->game_state.get());
				g->game_state->setup_game();

				games_.push_back(g);
				send_fn(json::parse(formatter() << "{ \"type\": \"game_created\", \"game_id\": " << g->game_state->game_id() << " }"));
			
				status_change();
				return;
			} else if(type == "observe_game") {
				const int id = msg["game_id"].as_int();
				const std::string user = msg["user"].as_string();
				const int session_id = msg["session_id"].as_int();

				game_info_ptr g;
				foreach(const game_info_ptr& gm, games_) {
					if(gm->game_state->game_id() == id) {
						g = gm;
						break;
					}
				}

				if(!g) {
					send_fn(json::parse("{ \"type\": \"unknown_game\" }"));
					return;
				}

				if(clients_.count(session_id)) {
					send_fn(json::parse("{ \"type\": \"reuse_session_id\" }"));
					return;
				}

				client_info& cli_info = clients_[session_id];
				cli_info.user = user;
				cli_info.game = g;
				cli_info.nplayer = -1;
				cli_info.last_contact = nheartbeat_;
				cli_info.session_id = session_id;

				g->clients.push_back(session_id);

				send_fn(json::parse(formatter() << "{ \"type\": \"observing_game\" }"));

				return;
			} else if(type == "get_status") {
				const int last_status = msg["last_seen"].as_int();
				if(last_status == status_id_) {
					status_fns_.push_back(send_fn);
				} else {
					send_fn(create_lobby_msg());
				}
				return;
			} else if(type == "get_server_info") {
				send_fn(get_server_info());
				return;
			} else {
				send_fn(json::parse("{ \"type\": \"unknown_message\" }"));
				return;
			}
		}

		std::map<int, client_info>::iterator client_itor = clients_.find(session_id);
		if(client_itor == clients_.end()) {
			std::cerr << "BAD SESSION ID: " << session_id << ": " << type << "\n";
			send_fn(json::parse("{ \"type\": \"invalid_session\" }"));
			return;
		}

		client_info& cli_info = client_itor->second;
		
		if(socket_info_fn) {
			socket_info& info = socket_info_fn();
			ASSERT_EQ(info.session_id, -1);
			info.nick = cli_info.user;
			info.session_id = session_id;
		}

		handle_message_internal(cli_info, msg);
		if(close_fn) {
			close_fn(cli_info);
		}
	}

	void server_base::status_change()
	{
		++status_id_;
		if(!status_fns_.empty()) {
			variant msg = create_lobby_msg();
			foreach(send_function send_fn, status_fns_) {
				send_fn(msg);
			}

			status_fns_.clear();
		}
	}

	variant server_base::create_lobby_msg() const
	{
		variant_builder value;
		value.add("type", "lobby");
		value.add("status_id", status_id_);

		std::vector<variant> games;
		foreach(game_info_ptr g, games_) {
			games.push_back(create_game_info_msg(g));
		}

		value.set("games", variant(&games));

		return value.build();
	}

	variant server_base::create_game_info_msg(game_info_ptr g) const
	{
		variant_builder value;
		value.add("type", "game_info");
		value.add("id", g->game_state->game_id());
		value.add("started", variant::from_bool(g->game_state->started()));

		size_t index = 0;
		std::vector<variant> clients;
		foreach(int cid, g->clients) {
			ASSERT_LOG(index < g->game_state->players().size(), "MIS-MATCHED INDEX: " << index << ", " << g->game_state->players().size());
			std::map<variant, variant> m;
			std::map<int, client_info>::const_iterator cinfo = clients_.find(cid);
			if(cinfo != clients_.end()) {
				m[variant("nick")] = variant(cinfo->second.user);
				m[variant("id")] = variant(cid);
				m[variant("bot")] = variant::from_bool(g->game_state->players()[index].is_human == false);
			}
			clients.push_back(variant(&m));
			++index;
		}

		value.set("clients", variant(&clients));

		return value.build();
	}

	void server_base::quit_games(int session_id)
	{
		client_info& cli_info = clients_[session_id];
		foreach(game_info_ptr& g, games_) {
			if(std::count(g->clients.begin(), g->clients.end(), session_id)) {
				const bool is_first_client = g->clients.front() == session_id;
				g->clients.erase(std::remove(g->clients.begin(), g->clients.end(), session_id), g->clients.end());

				if(!g->game_state->started()) {
					g->game_state->remove_player(cli_info.user);
					if(is_first_client) {
						g->clients.clear();
						//TODO: remove joining clients from the game nicely.
					} else {
						const std::string msg = create_game_info_msg(g).write_json(true, variant::JSON_COMPLIANT);
						foreach(int client, g->clients) {
							queue_msg(client, msg);
						}
					}
				} else if(g->game_state->get_player_index(cli_info.user) != -1) {
					std::cerr << "sending quit message...\n";
					g->game_state->queue_message(formatter() << "<message text=\"" << cli_info.user << " has quit\"/>");
					flush_game_messages(*g);
				}

				if(g->clients.empty()) {
					//game has no more clients left, so kill it.
					g.reset();
				}
			}
		}

		int games_size = games_.size();

		games_.erase(std::remove(games_.begin(), games_.end(), game_info_ptr()), games_.end());

		cli_info.game.reset();

		if(games_size != games_.size()) {
			status_change();
		}
	}

	void server_base::flush_game_messages(game_info& info)
	{
		std::vector<game::message> game_response;
		info.game_state->swap_outgoing_messages(game_response);
		foreach(game::message& msg, game_response) {
			if(msg.recipients.empty()) {
				foreach(int session_id, info.clients) {
					if(session_id != -1) {
						queue_msg(session_id, msg.contents);
					}
				}
			} else {
				foreach(int player, msg.recipients) {
					if(player >= static_cast<int>(info.clients.size())) {
						//Might be an observer who since disconnected.
						continue;
					}

					if(player >= 0) {
						queue_msg(info.clients[player], msg.contents);
					} else {
						//A message for observers
						for(size_t n = info.game_state->players().size(); n < info.clients.size(); ++n) {
							queue_msg(info.clients[n], msg.contents);
						}
					}
				}
			}
		}
	}

	void server_base::schedule_write()
	{
		if(scheduled_write_) {
			return;
		}

		scheduled_write_ = nheartbeat_ + 10;
	}

	void server_base::handle_message_internal(client_info& cli_info, const variant& msg)
	{
		const std::string& user = cli_info.user;
		const std::string& type = msg["type"].as_string();

		cli_info.last_contact = nheartbeat_;

		if(cli_info.game) {
			if(type == "quit") {
				quit_games(cli_info.session_id);
				queue_msg(cli_info.session_id, "{ \"type\": \"bye\" }");

				return;
			}

			const bool game_started = cli_info.game->game_state->started();
			const game_context context(cli_info.game->game_state.get());

			cli_info.game->nlast_touch = nheartbeat_;
			cli_info.game->game_state->handle_message(cli_info.nplayer, msg);
			flush_game_messages(*cli_info.game);
		}
	}

	void server_base::queue_msg(int session_id, const std::string& msg, bool has_priority)
	{
		if(session_id == -1) {
			return;
		}

		if(has_priority) {
			clients_[session_id].msg_queue.push_front(msg);
		} else {
			clients_[session_id].msg_queue.push_back(msg);
		}
	}

	void server_base::heartbeat(const boost::system::error_code& error)
	{
		if(error == boost::asio::error::operation_aborted) {
			std::cerr << "tbs_server::heartbeat cancelled" << std::endl;
			return;
		}
		timer_.expires_from_now(boost::posix_time::milliseconds(100));
		timer_.async_wait(boost::bind(&server_base::heartbeat, this, 
			boost::asio::placeholders::error));

		foreach(game_info_ptr g, games_) {
			g->game_state->process();
		}

		nheartbeat_++;
		if(nheartbeat_%10 != 0) {
			return;
		}

		foreach(game_info_ptr& g, games_) {
			if(g->nlast_touch > nheartbeat_ + 300) {
				g = game_info_ptr();
			}
		}

		games_.erase(std::remove(games_.begin(), games_.end(), game_info_ptr()), games_.end());

		for(std::map<int,client_info>::iterator i = clients_.begin();
		    i != clients_.end(); ) {
			if(i->second.game->nlast_touch > nheartbeat_ + 300) {
				clients_.erase(i++);
			} else {
				++i;
			}
		}

#if !defined(__ANDROID__)
		sys::pump_file_modifications();
#endif

		const bool send_heartbeat = nheartbeat_%100 == 0;

		heartbeat_internal(send_heartbeat, clients_);

		if(send_heartbeat) {
			status_change();
		}

		if(nheartbeat_ >= scheduled_write_) {
			//write out the game recorded data.
		}
	}

	variant server_base::create_heartbeat_packet(const client_info& cli_info)
	{
		variant_builder doc;
		doc.add("type", "heartbeat");

		std::vector<variant> items;

		foreach(int client_session, cli_info.game->clients) {
			const client_info& info = clients_[client_session];
			variant_builder value;

			value.add("nick", info.user);
			value.add("ingame", info.game == cli_info.game);
			value.add("lag", nheartbeat_ - info.last_contact);

			items.push_back(value.build());
		}

		foreach(const std::string& ai, cli_info.game->game_state->get_ai_players()) {
			variant_builder value;

			value.add("nick", ai);
			value.add("ingame", true);
			value.add("lag", 0);

			items.push_back(value.build());
		}

		doc.set("players", variant(&items));
		return doc.build();
	}
}
