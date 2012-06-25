#include <algorithm>
#include <iostream>
#include <string>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "tbs_server.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

namespace tbs {

server::game_info::game_info(const variant& value)
{
	game_state = game::create(value);
}

server::client_info::client_info() : nplayer(0), last_contact(0)
{}

server::server(boost::asio::io_service& io_service)
  : timer_(io_service), nheartbeat_(0), scheduled_write_(0), status_id_(0)
{
	heartbeat();
}

void server::adopt_ajax_socket(socket_ptr socket, int session_id, const variant& msg)
{
	const std::string& type = msg["type"].as_string();

	if(session_id == -1) {
		if(type == "create_game") {
			game_info_ptr g(new game_info(msg));
			if(!g->game_state) {
				std::cerr << "COULD NOT CREATE GAME TYPE: " << msg["game_type"].as_string() << "\n";
				send_msg(socket, "{ \"type\": \"create_game_failed\" }");
				return;
			}

			std::vector<variant> users = msg["users"].as_list();
			for(int i = 0; i != users.size(); ++i) {
				const std::string user = users[i]["user"].as_string();
				const int session_id = users[i]["session_id"].as_int();

				if(clients_.count(session_id) && session_id != -1) {
					std::cerr << "ERROR: REUSED SESSION ID WHEN CREATING GAME: " << session_id << "\n";
					send_msg(socket, "{ \"type\": \"create_game_failed\" }");
					return;
				}

				if(session_id != -1) {
					client_info& cli_info = clients_[session_id];
					cli_info.user = user;
					cli_info.game = g;
					cli_info.nplayer = i;
					cli_info.last_contact = nheartbeat_;
					cli_info.session_id = session_id;

					g->game_state->add_player(user);
				} else {
					g->game_state->add_ai_player(user);
				}

				g->clients.push_back(session_id);
			}

			const game_context context(g->game_state.get());
			g->game_state->setup_game();

			games_.push_back(g);
			send_msg(socket, formatter() << "{ \"type\": \"game_created\", \"game_id\": " << g->game_state->game_id() << " }");
			
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
				send_msg(socket, "{ \"type\": \"unknown_game\" }");
				return;
			}

			if(clients_.count(session_id)) {
				send_msg(socket, "{ \"type\": \"reuse_session_id\" }");
				return;
			}

			client_info& cli_info = clients_[session_id];
			cli_info.user = user;
			cli_info.game = g;
			cli_info.nplayer = -1;
			cli_info.last_contact = nheartbeat_;
			cli_info.session_id = session_id;

			g->clients.push_back(session_id);

			send_msg(socket, formatter() << "{ \"type\": \"observing_game\" }");

			return;
		} else if(type == "get_status") {
			const int last_status = msg["last_seen"].as_int();
			if(last_status == status_id_) {
				status_sockets_.push_back(socket);
			} else {
				send_msg(socket, create_lobby_msg().write_json());
			}
			return;
		} else {
			send_msg(socket, "{ \"type\": \"unknown_message\" }");
			return;
		}
	}

	std::map<int, client_info>::iterator client_itor = clients_.find(session_id);
	if(client_itor == clients_.end()) {
		std::cerr << "BAD SESSION ID: " << session_id << "\n";
		send_msg(socket, "{ \"type\": \"invalid_session\" }");
		return;
	}

	client_info& cli_info = client_itor->second;

	socket_info& info = connections_[socket];
	info.nick = cli_info.user;

	handle_message_internal(socket, cli_info, msg);
	close_ajax(socket, cli_info);
}

void server::handle_message_internal(socket_ptr socket, client_info& cli_info, const variant& msg)
{
	const std::string& user = cli_info.user;
	const std::string& type = msg["type"].as_string();

	std::cerr << "GOT MESSAGE: '" << type << "'\n";

	cli_info.last_contact = nheartbeat_;

	if(cli_info.game) {
		if(type == "quit") {
			quit_games(cli_info.session_id);
			queue_msg(cli_info.session_id, "{ \"type\": \"bye\" }");

			return;
		}

		const bool game_started = cli_info.game->game_state->started();
		const game_context context(cli_info.game->game_state.get());

		std::cerr << "MESSAGE FROM " << cli_info.session_id << " -> " << cli_info.nplayer << "\n";
		cli_info.game->game_state->handle_message(cli_info.nplayer, msg);
		flush_game_messages(*cli_info.game);
	}
}

void server::close_ajax(socket_ptr socket, client_info& cli_info)
{
	socket_info& info = connections_[socket];

	if(cli_info.msg_queue.empty() == false) {

		for(std::map<socket_ptr,std::string>::iterator s = waiting_connections_.begin();
		    s != waiting_connections_.end(); ++s) {
			if(s->second == info.nick && s->first != socket) {
				send_msg(s->first, "{ \"type\": \"keepalive\" }");
				break;
			}
		}

		send_msg(socket, cli_info.msg_queue.front());
		cli_info.msg_queue.pop_front();
	} else {
		waiting_connections_[socket] = info.nick;
	}
}

void server::queue_msg(int session_id, const std::string& msg)
{
	if(session_id == -1) {
		return;
	}

	clients_[session_id].msg_queue.push_back(msg);
}

void server::send_msg(socket_ptr socket, const variant& msg)
{
	send_msg(socket, msg.write_json());
}

void server::send_msg(socket_ptr socket, const char* msg)
{
	send_msg(socket, std::string(msg));
}

void server::send_msg(socket_ptr socket, const std::string& msg)
{
	const socket_info& info = connections_[socket];
	char buf[4096];
	sprintf(buf, "HTTP/1.1 200 OK\nDate: Tue, 20 Sep 2011 21:00:00 GMT\nConnection: close\nServer: Wizard/1.0\nAccept-Ranges: bytes\nAccess-Control-Allow-Origin: *\nContent-Type: application/json\nContent-Length: %d\nLast-Modified: Tue, 20 Sep 2011 10:00:00 GMT\n\n", (int)msg.size());
	std::string header = buf;

	boost::shared_ptr<std::string> str_buf(new std::string(header.empty() ? msg : (header + msg)));
	boost::asio::async_write(*socket, boost::asio::buffer(*str_buf),
			                         boost::bind(&server::handle_send, this, socket, _1, _2, str_buf));
	std::cerr << "SEND MSG: " << *str_buf << "\n";
}

void server::handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, boost::shared_ptr<std::string> buf)
{
	if(e) {
		std::cerr << "ERROR SENDING DATA\n";
	}

	disconnect(socket);
}

void server::disconnect(socket_ptr socket)
{
	waiting_connections_.erase(socket);
	connections_.erase(socket);
	socket->close();
}

void server::heartbeat()
{
	const bool send_heartbeat = nheartbeat_++%10 == 0;

	for(std::map<socket_ptr, std::string>::iterator i = waiting_connections_.begin(); i != waiting_connections_.end(); ++i) {
		socket_ptr socket = i->first;

		socket_info& info = connections_[socket];
		client_info& cli_info = clients_[info.session_id];
		if(cli_info.msg_queue.empty() == false) {
			send_msg(socket, cli_info.msg_queue.front());
			cli_info.msg_queue.pop_front();
		} else if(send_heartbeat) {
			if(!cli_info.game) {
				send_msg(socket, "{ \"type\": \"heartbeat\" }");
			} else {
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

				send_msg(socket, doc.build());
			}
		}
	}

	if(send_heartbeat) {
		status_change();
	}

	if(nheartbeat_ >= scheduled_write_) {
		//write out the game recorded data.
	}

	timer_.expires_from_now(boost::posix_time::seconds(1));
	timer_.async_wait(boost::bind(&server::heartbeat, this));
}

void server::schedule_write()
{
	if(scheduled_write_) {
		return;
	}

	scheduled_write_ = nheartbeat_ + 10;
}

void server::flush_game_messages(game_info& info)
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
					for(int n = info.game_state->players().size(); n < info.clients.size(); ++n) {
						queue_msg(info.clients[n], msg.contents);
					}
				}
			}
		}
	}
}

void server::quit_games(int session_id)
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
					const std::string msg = create_game_info_msg(g).write_json();
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

variant server::create_lobby_msg() const
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

variant server::create_game_info_msg(game_info_ptr g) const
{
	variant_builder value;
	value.add("type", "game_info");
	value.add("id", g->game_state->game_id());
	value.add("started", g->game_state->started());

	return value.build();
}

void server::status_change()
{
	++status_id_;
	if(!status_sockets_.empty()) {
		std::string msg = create_lobby_msg().write_json();
		foreach(socket_ptr socket, status_sockets_) {
			send_msg(socket, msg);
		}

		status_sockets_.clear();
	}
}

}
