#include <algorithm>
#include <iostream>
#include <string>
#include <ctime>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "module.hpp"
#include "json_parser.hpp"
#include "tbs_server.hpp"
#include "string_utils.hpp"
#include "utils.hpp"
#include "variant_utils.hpp"


namespace tbs {

namespace 
{
	const variant& get_server_info()
	{
		static variant server_info;
		if(server_info.is_null()) {
			server_info = json::parse_from_file("data/server_info.cfg");
			server_info.add_attr(variant("type"), variant("server_info"));
		}
		return server_info;
	}
}

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
	fprintf(stderr, "DEBUG: adopt_ajax_socket(%p -> %d)\n", socket.get(), session_id);
	const std::string& type = msg["type"].as_string();

	if(session_id == -1) {
		if(type == "create_game") {
			game_info_ptr g(new game_info(msg));
			if(!g->game_state) {
				std::cerr << "COULD NOT CREATE GAME TYPE: " << msg["game_type"].as_string() << "\n";
				fprintf(stderr, "DEBUG: send_msg(a)\n");
				send_msg(socket, "{ \"type\": \"create_game_failed\" }");
				return;
			}

			std::vector<variant> users = msg["users"].as_list();
			for(int i = 0; i != users.size(); ++i) {
				const std::string user = users[i]["user"].as_string();
				const int session_id = users[i]["session_id"].as_int();

				if(clients_.count(session_id) && session_id != -1) {
					std::cerr << "ERROR: REUSED SESSION ID WHEN CREATING GAME: " << session_id << "\n";
				fprintf(stderr, "DEBUG: send_msg(b)\n");
					send_msg(socket, "{ \"type\": \"create_game_failed\" }");
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
				fprintf(stderr, "DEBUG: send_msg(c)\n");
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
				fprintf(stderr, "DEBUG: send_msg(d)\n");
				send_msg(socket, "{ \"type\": \"unknown_game\" }");
				return;
			}

			if(clients_.count(session_id)) {
				fprintf(stderr, "DEBUG: send_msg(e)\n");
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

				fprintf(stderr, "DEBUG: send_msg(f)\n");
			send_msg(socket, formatter() << "{ \"type\": \"observing_game\" }");

			return;
		} else if(type == "get_status") {
			const int last_status = msg["last_seen"].as_int();
			if(last_status == status_id_) {
				status_sockets_.push_back(socket);
			} else {
				fprintf(stderr, "DEBUG: send_msg(g)\n");
				send_msg(socket, create_lobby_msg().write_json(true, variant::JSON_COMPLIANT));
			}
			return;
		} else if(type == "get_server_info") {
				fprintf(stderr, "DEBUG: send_msg(h)\n");
			send_msg(socket, get_server_info().write_json(true, variant::JSON_COMPLIANT));
		} else {
				fprintf(stderr, "DEBUG: send_msg(i)\n");
			send_msg(socket, "{ \"type\": \"unknown_message\" }");
			return;
		}
	}

	std::map<int, client_info>::iterator client_itor = clients_.find(session_id);
	if(client_itor == clients_.end()) {
		std::cerr << "BAD SESSION ID: " << session_id << "\n";
				fprintf(stderr, "DEBUG: send_msg(j)\n");
		send_msg(socket, "{ \"type\": \"invalid_session\" }");
		return;
	}

	client_info& cli_info = client_itor->second;

	socket_info& info = connections_[socket];
	info.nick = cli_info.user;

	handle_message_internal(socket, cli_info, msg);
	close_ajax(socket, cli_info);
}

void server::clear_games()
{
	games_.clear();
	clients_.clear();
}

void server::handle_message_internal(socket_ptr socket, client_info& cli_info, const variant& msg)
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

		cli_info.game->game_state->handle_message(cli_info.nplayer, msg);
		flush_game_messages(*cli_info.game);
	}
}

void server::close_ajax(socket_ptr socket, client_info& cli_info)
{
	socket_info& info = connections_[socket];

	if(cli_info.msg_queue.empty() == false) {
		std::vector<socket_ptr> keepalive_sockets;

		for(std::map<socket_ptr,std::string>::iterator s = waiting_connections_.begin();
		    s != waiting_connections_.end(); ) {
			if(s->second == info.nick && s->first != socket) {
				fprintf(stderr, "DEBUG: send_msg(k)\n");
				keepalive_sockets.push_back(s->first);
				if(sessions_to_waiting_connections_.count(cli_info.session_id)) {
				fprintf(stderr, "DEBUG: sessions_to_waiting_connections_.erase(%d -> %p)\n", cli_info.session_id, sessions_to_waiting_connections_.find(cli_info.session_id)->second.get());
				}
				sessions_to_waiting_connections_.erase(cli_info.session_id);
				waiting_connections_.erase(s++);
			} else {
				++s;
			}
		}

				fprintf(stderr, "DEBUG: send_msg(l)\n");
		const std::string msg = cli_info.msg_queue.front();
		cli_info.msg_queue.pop_front();
		send_msg(socket, msg);

		foreach(socket_ptr socket, keepalive_sockets) {
			send_msg(socket, "{ \"type\": \"keepalive\" }");
		}
	} else {
		waiting_connections_[socket] = info.nick;
		fprintf(stderr, "DEBUG: A sessions_to_waiting_connections_.insert(%d -> %p)\n", cli_info.session_id, socket.get());
		sessions_to_waiting_connections_[cli_info.session_id] = socket;
	}
}

void server::queue_msg(int session_id, const std::string& msg, bool has_priority)
{
	if(session_id == -1) {
		return;
	}

	std::map<int, socket_ptr>::iterator itor = sessions_to_waiting_connections_.find(session_id);
	if(itor != sessions_to_waiting_connections_.end()) {
		const int session_id = itor->first;
		const socket_ptr sock = itor->second;
		waiting_connections_.erase(sock);
		fprintf(stderr, "DEBUG: B sessions_to_waiting_connections_.erase(%d -> %p)\n", itor->first, itor->second.get());
		sessions_to_waiting_connections_.erase(session_id);
				fprintf(stderr, "DEBUG: send_msg(m)\n");
		send_msg(sock, msg);
		return;
	}

	if(has_priority) {
		clients_[session_id].msg_queue.push_front(msg);
	} else {
		clients_[session_id].msg_queue.push_back(msg);
	}
}

void server::send_msg(socket_ptr socket, const variant& msg)
{
	send_msg(socket, msg.write_json(true, variant::JSON_COMPLIANT));
}

void server::send_msg(socket_ptr socket, const char* msg)
{
	send_msg(socket, std::string(msg));
}

void server::send_msg(socket_ptr socket, const std::string& msg)
{
	fprintf(stderr, "DEBUG: send_msg(%p)\n", socket.get());
	const socket_info& info = connections_[socket];
	std::stringstream buf;
	buf <<
		"HTTP/1.1 200 OK\r\n"
		"Date: " << get_http_datetime() << "\r\n"
		"Connection: close\r\n"
		"Server: Wizard/1.0\r\n"
		"Accept-Ranges: bytes\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: " << std::dec << (int)msg.size() << "\r\n"
		"Last-Modified: " << get_http_datetime() << "\r\n\r\n";
	std::string header = buf.str();

	boost::shared_ptr<std::string> str_buf(new std::string(header.empty() ? msg : (header + msg)));
	fprintf(stderr, "DEBUG: CALL async_write()\n");
	boost::asio::async_write(*socket, boost::asio::buffer(*str_buf),
			                         boost::bind(&server::handle_send, this, socket, _1, _2, str_buf, info.session_id));
	fprintf(stderr, "DEBUG: DONE async_write()\n");
}

void server::handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, boost::shared_ptr<std::string> buf, int session_id)
{
	fprintf(stderr, "DEBUG: handle_send(%p)\n", socket.get());
	if(e) {
		std::cerr << "ERROR SENDING DATA: " << e.message() << std::endl;
		queue_msg(session_id, *buf, true); //re-queue the message.
	}

	disconnect(socket);
}

std::vector<socket_ptr> debug_disconnected_store;

void server::disconnect(socket_ptr socket)
{
	debug_disconnected_store.push_back(socket);

	fprintf(stderr, "DEBUG: disconnect(%p)\n", socket.get());
	std::map<socket_ptr, socket_info>::iterator itor = connections_.find(socket);
	if(itor != connections_.end()) {
		std::map<int, socket_ptr>::iterator sessions_itor = sessions_to_waiting_connections_.find(itor->second.session_id);
		if(sessions_itor != sessions_to_waiting_connections_.end() && sessions_itor->second == socket) {
		fprintf(stderr, "DEBUG: C sessions_to_waiting_connections_.erase(%d -> %p)\n", sessions_itor->first, sessions_itor->second.get());
			sessions_to_waiting_connections_.erase(sessions_itor);
		}

		connections_.erase(itor);
	}

	waiting_connections_.erase(socket);

	for(std::map<int, socket_ptr>::iterator itor = sessions_to_waiting_connections_.begin(); itor != sessions_to_waiting_connections_.end(); ++itor) {
		if(itor->second == socket) {
			fprintf(stderr, "DEBUG: socket in sessions_to_waiting_connections %p -> %d\n", socket.get(), itor->first);
		}
	}

	for(std::map<socket_ptr, std::string>::iterator itor = waiting_connections_.begin(); itor != waiting_connections_.end(); ++itor) {
		if(itor->first == socket) {
			fprintf(stderr, "DEBUG: socket in waiting_connections_ %p\n", socket.get());
		}
	}

	foreach(socket_ptr s, status_sockets_) {
			fprintf(stderr, "DEBUG: socket in status_sockets %p\n", socket.get());
	}

	socket->close();
}

void server::heartbeat()
{
	timer_.expires_from_now(boost::posix_time::milliseconds(100));
	timer_.async_wait(boost::bind(&server::heartbeat, this));

	foreach(game_info_ptr g, games_) {
		g->game_state->process();
	}

	nheartbeat_++;
	if(nheartbeat_%10 != 0) {
		return;
	}

#if !defined(__ANDROID__)
	sys::pump_file_modifications();
#endif

	fprintf(stderr, "DEBUG: BEGIN HEARTBEAT\n");

	const bool send_heartbeat = nheartbeat_%100 == 0;

	std::vector<std::pair<socket_ptr, std::string> > messages;

	for(std::map<socket_ptr, std::string>::iterator i = waiting_connections_.begin(); i != waiting_connections_.end(); ++i) {
		socket_ptr socket = i->first;

		socket_info& info = connections_[socket];
		client_info& cli_info = clients_[info.session_id];
		fprintf(stderr, "DEBUG: SOCKET %p -> %d\n", socket.get(), info.session_id);
		if(cli_info.msg_queue.empty() == false) {
			messages.push_back(std::pair<socket_ptr,std::string>(socket, cli_info.msg_queue.front()));
			cli_info.msg_queue.pop_front();

		std::map<int, socket_ptr>::iterator sessions_itor = sessions_to_waiting_connections_.find(info.session_id);
		if(sessions_itor != sessions_to_waiting_connections_.end()) {
		fprintf(stderr, "DEBUG: F sessions_to_waiting_connections_.erase(%d -> %p)\n", sessions_itor->first, sessions_itor->second.get());
		}
			sessions_to_waiting_connections_.erase(info.session_id);
		} else if(send_heartbeat) {
			if(!cli_info.game) {
				messages.push_back(std::pair<socket_ptr,std::string>(socket, "{ \"type\": \"heartbeat\" }"));
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

				fprintf(stderr, "DEBUG: send_msg(p)\n");
				messages.push_back(std::pair<socket_ptr,std::string>(socket, doc.build().write_json()));
			}

		std::map<int, socket_ptr>::iterator sessions_itor = sessions_to_waiting_connections_.find(info.session_id);
		if(sessions_itor != sessions_to_waiting_connections_.end()) {
		fprintf(stderr, "DEBUG: G sessions_to_waiting_connections_.erase(%d -> %p)\n", sessions_itor->first, sessions_itor->second.get());
		}
			sessions_to_waiting_connections_.erase(info.session_id);
		}
	}

	for(int i = 0; i != messages.size(); ++i) {
		waiting_connections_.erase(messages[i].first);
	}

	for(int i = 0; i != messages.size(); ++i) {
		send_msg(messages[i].first, messages[i].second);
	}

	if(send_heartbeat) {
		status_change();
	}

	fprintf(stderr, "DEBUG: END HEARTBEAT\n");

	if(nheartbeat_ >= scheduled_write_) {
		//write out the game recorded data.
	}
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
					for(size_t n = info.game_state->players().size(); n < info.clients.size(); ++n) {
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

void server::status_change()
{
	++status_id_;
	if(!status_sockets_.empty()) {
		std::string msg = create_lobby_msg().write_json(true, variant::JSON_COMPLIANT);
		foreach(socket_ptr socket, status_sockets_) {
				fprintf(stderr, "DEBUG: send_msg(q)\n");
			send_msg(socket, msg);
		}

		status_sockets_.clear();
	}
}

}
