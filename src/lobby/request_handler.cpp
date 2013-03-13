//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/lexical_cast.hpp>

#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

#include "asserts.hpp"
#include "connection.hpp"
#include "request_handler.hpp"
#include "mime_types.hpp"
#include "name.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "game_server_worker.hpp"

namespace http {
namespace server {

namespace
{
	const int64_t game_server_wait_interval_ms = 30000;

	enum start_game_errors
	{
		game_created,
		max_player_size_exceeded,
		not_enough_human_players,
		game_server_timed_out,
		game_server_failed_create,
		game_server_bad_response,
	};

	bool process_start_game_message(const std::string& game_type, 
		const json_spirit::mArray& players, 
		size_t bots, 
		const json_spirit::mArray& bot_type,
		json_spirit::mObject& reply_object)
	{
		json_spirit::mObject m;
		game_server::worker& w = game_server::worker::get_server_from_game_type(game_type);
		const game_server::server_info& si = w.get_server_info();
		if(players.size() > si.max_players) {
			reply_object["type"] = "error";
			reply_object["description"] = "Maximum number of human players exceeded.";
			return false;
		}
		if(players.size() < si.min_humans) {
			reply_object["type"] = "error";
			reply_object["description"] = "Not enough human players supplied.";
			return false;
		}
		if(players.size() + bots < si.min_players) {
			bots = si.min_players - players.size();
		}
		if(players.size() + bots > si.max_players) {
			bots = si.max_players - players.size();
		}
		if(bots != bot_type.size()) {
			reply_object["type"] = "error";
			reply_object["description"] = "Not enough bot types specified.";
			return false;
		}
		json_spirit::mArray u_ary;
		// {type: 'create_game', game_type: 'citadel', users: [{user: 'a', session_id: 1}, {user: 'b', bot: true, session_id: 2}]}
		// Add Bots
		u_ary.insert(u_ary.end(), players.begin(), players.end());
		for(size_t n = 0; n != bots; ++n) {
			json_spirit::mObject b_obj;
			b_obj["user"] = name::generate_bot_name();
			b_obj["bot"] = true;
			b_obj["bot_type"] = bot_type[n].get_str(); 
			b_obj["session_id"] = game_server::shared_data::make_session_id();
			u_ary.push_back(b_obj);
		}
		m["type"] = "create_game";
		m["game_type"] = game_type;
		m["users"] = u_ary;
		// Push 'create_game' message to the game server.
		game_server::message msg;
		msg.msg = json_spirit::write(m);
		std::cerr << "Writing message to game_server: " << msg.msg << std::endl;
		msg.reply = game_server::string_queue_ptr(new game_server::string_queue);
		w.get_queue()->push(msg);
		std::string reply;
		if(msg.reply->wait_and_pop(reply, game_server_wait_interval_ms)) {
			json_spirit::mValue rv;
			json_spirit::read(reply, rv);
			json_spirit::mObject ro = rv.get_obj();
			if(ro.find("type") != ro.end()) {
				const std::string& type = ro["type"].get_str();
				if(type == "create_game_failed") {
					reply_object["type"] = "error";
					reply_object["description"] = "Server replied: Create game failed.";
				} else if(type == "game_created") {
					for(auto it : ro) {
						reply_object[it.first] = it.second;
					}
					if(reply_object.find("game_server_address") == reply_object.end()) {
						reply_object["game_server_address"] = si.server_address;
					}
					if(reply_object.find("game_server_port") == reply_object.end()) {
						reply_object["game_server_port"] = si.server_port;
					}
					return true;
				} else {
					reply_object["type"] = "error";
					reply_object["description"] = "Didn't understand server response. 'type' unknown: " + type;
					return false;
				}
			} else {
				reply_object["type"] = "error";
				reply_object["description"] = "Didn't understand server response. No 'type' attribute";
				return false;
			}
		} else {
			// XXX we could really do a fail-over and retry the another server with this.
			reply_object["type"] = "error";
			reply_object["description"] = "Timeout waiting for game server to reply.";
			return false;
		}
		ASSERT_LOG(false, "Entered code path not accessible");
		return false;
	}
}

request_handler::request_handler(const std::string& doc_root, game_server::shared_data& data)
  : doc_root_(doc_root), data_(data)
{
}

bool request_handler::handle_request(const request& req, reply& rep, http::server::connection_ptr conn)
{
	if(req.method == "POST") {
		return handle_post(req, rep, conn);
	} else if(req.method == "GET") {
		return handle_get(req, rep, conn);
	} else {
		// handled
		std::cerr << "Unhandled request: " << req.method << std::endl;
		rep = reply::stock_reply(reply::not_implemented);
	}
	return true;
}

bool request_handler::handle_get(const request& req, reply& rep, http::server::connection_ptr conn)
{
  // Decode url to path.
  std::string request_path; 
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return true;
  }

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return true;
  }

  // If path ends in slash (i.e. is a directory) then add "index.html".
  if (request_path[request_path.size() - 1] == '/')
  {
    request_path += "index.html";
  }

  // Determine the file extension.
  std::size_t last_slash_pos = request_path.find_last_of("/");
  std::size_t last_dot_pos = request_path.find_last_of(".");
  std::string extension;
  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
  {
    extension = request_path.substr(last_dot_pos + 1);
  }

  // Open the file to send back.
  std::string full_path = doc_root_ + request_path;
  std::unique_ptr<std::istream> is = std::unique_ptr<std::istream>(new std::ifstream(full_path.c_str(), std::ios::in | std::ios::binary));
  if(is == nullptr || !(*is))
  {
	rep = reply::stock_reply(reply::not_found);
	return true;
  }

  // Fill out the reply to be sent to the client.
  rep.status = reply::ok;
  char buf[512];
  while (is->read(buf, sizeof(buf)).gcount() > 0)
    rep.content.append(buf, unsigned(is->gcount()));
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = mime_types::extension_to_type(extension);
  return true;
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

bool request_handler::check_messages(const std::string& user, reply& rep, http::server::connection_ptr conn)
{
	game_server::client_message_queue_ptr msg_q = data_.get_message_queue(user);
	json_spirit::mValue mv;
	if(msg_q && msg_q->try_pop(mv)) {
		return reply::create_json_reply(mv, rep);
	}
	// set the connection waiting for a reply.
	data_.set_waiting_connection(user, conn);
	return false;
}

bool request_handler::handle_post(const request& req, reply& rep, http::server::connection_ptr conn)
{
	json_spirit::mObject obj;
	if(req.uri != "/tbs") {
		rep = reply::stock_reply(reply::not_found);
		return true;
	}
	std::cerr << "POST /tbs" << std::endl;
	std::cerr << "BODY: " << req.body << std::endl;

	json_spirit::mValue value;
	json_spirit::read(req.body, value);

	if(value.type() != json_spirit::obj_type) {
		rep = reply::bad_request_with_detail("Not valid JSON data. Data was: <pre>" + req.body + "</pre>");
		return true;
	}
	auto req_obj = value.get_obj();
	auto it = req_obj.find("type");
	if(it == req_obj.end()) {
		obj["type"] = "error";
		obj["description"] = "No 'type' field in request";
		return reply::create_json_reply(json_spirit::mValue(obj), rep);
	}
	const std::string& type = it->second.get_str();

	if(type == "lobby_get_users") {
		json_spirit::mArray ua;
		data_.get_user_list(&ua);
		obj["type"] = "lobby_users";
		obj["users"] = ua;
		return reply::create_json_reply(json_spirit::mValue(obj), rep);
	}

	if(type == "lobby_get_server_info") {
		json_spirit::mObject si_obj;
		game_server::worker::get_server_info(&si_obj);
		obj["type"] = "lobby_server_info";
		obj["servers"] = si_obj;
		return reply::create_json_reply(json_spirit::mValue(obj), rep);
	}

	int session_id = -1;
	it = req_obj.find("session_id");
	if(it != req_obj.end()) {
		if(it->second.type() == json_spirit::int_type) {
			session_id = it->second.get_int();
		} else {
			obj["type"] = "error";
			obj["error"] = "'session_id' attribute must be integer type";
			return reply::create_json_reply(json_spirit::mValue(obj), rep);
		}
	}

	it = req_obj.find("user");
	if(it == req_obj.end()) {
		obj["type"] = "error";
		obj["error"] = "No 'user' field in request";
		return reply::create_json_reply(json_spirit::mValue(obj), rep);
	}
	if(it->second.type() != json_spirit::str_type) {
		obj["type"] = "error";
		obj["error"] = "'user' attribute must be string type";
		return reply::create_json_reply(json_spirit::mValue(obj), rep);
	}
	const std::string& user = it->second.get_str();
	if(user.empty()) {
		obj["type"] = "error";
		obj["error"] = "'user' field is empty.";
		return reply::create_json_reply(json_spirit::mValue(obj), rep);
	}

	std::string pass;
	it = req_obj.find("password");
	if(it != req_obj.end()) {
		if(it->second.type() == json_spirit::str_type) {
			pass = it->second.get_str();
		}
	}

	if(type == "lobby_login") {
		game_server::shared_data::action action;
		game_server::client_info ci;
		boost::tie(action, ci) = data_.process_user(user, pass, session_id);
		if(action == game_server::shared_data::bad_session_id) {
			obj["type"] = "error";
			obj["description"] = "Bad session ID";
		} else if(action == game_server::shared_data::send_salt) {
			obj["type"] = "lobby_password_request";
			obj["salt"] = ci.salt;
			obj["session_id"] = ci.session_id;
		} else if(action == game_server::shared_data::user_not_found) {
			obj["type"] = "error";
			obj["description"] = "User not found";
		} else if(action == game_server::shared_data::password_failed) {
			obj["type"] = "error";
			obj["description"] = "Password failed.";
		} else if(action == game_server::shared_data::login_success) {
			//obj["type"] = "lobby_login_success";
			//obj["session_id"] = ci.session_id;
			// Post message to all the other users that someone new is on.
			json_spirit::mObject mobj;
			mobj["type"] = "lobby_user_login";
			mobj["user"] = user;
			data_.post_message_to_all_clients(json_spirit::mValue(mobj));
			return check_messages(user, rep, conn);
		} else {
			ASSERT_LOG(false, "Unhandled state: " << static_cast<int>(action));
		}
	} else if(type == "lobby_create_game") {
		// waiting for other players
		auto gt_it = req_obj.find("game_type");
		if(gt_it == req_obj.end()) {
			obj["type"] = "error";
			obj["description"] = "No 'game_type' attribute given.";
		} else {
			const std::string& game_type = gt_it->second.get_str();
			int game_id;
			if(data_.create_game(user, game_type, &game_id)) {
				// Send game_created message to other waiting clients (or send lobby status update)
				json_spirit::mObject game_created_obj;
				game_created_obj["type"] = "lobby_game_created";
				game_created_obj["user"] = user;
				game_created_obj["game_id"] = game_id;
				game_created_obj["game_type"] = game_type;
				data_.post_message_to_all_clients(json_spirit::mValue(game_created_obj));
				return check_messages(user, rep, conn);
			} else {
				obj["type"] = "error";
				obj["description"] = "Unable to create game.";
			}
		}
	} else if(type == "lobby_start_game") {
		// Sending the "create_game" message to the tbs server
		int game_id = req_obj["game_id"].get_int();
		if(data_.is_user_in_game(user, game_id)) {
			const game_server::game_info* gi = data_.get_game_info(game_id);
			if(gi) {
				json_spirit::mArray players;
				for(auto pit : gi->clients) {
					json_spirit::mObject pobj;
					pobj["user"] = pit;
					pobj["session_id"] = data_.get_user_session_id(pit);
					players.push_back(pobj);
				}

				if(process_start_game_message(gi->name, 
					players, 
					gi->bot_count, 
					json_spirit::mArray(gi->bot_types.begin(), gi->bot_types.end()), obj)) {
					data_.post_message_to_game_clients(game_id, json_spirit::mValue(obj));
				return check_messages(user, rep, conn);
				}
			} else {
				obj["type"] = "error";
				obj["description"] = "No game_info structure available for game_id";
			}
		} else {
			obj["type"] = "error";
			obj["description"] = "user is not in game with given identifier";
		}
	} else if(type == "lobby_quit") {
		if(data_.sign_off(user, session_id)) {
			// check if user was in any games and notify based on that.
			int game_id;
			if(!data_.check_client_in_games(user, &game_id)) {
				// Still users in game post a message to them
				json_spirit::mObject player_left_obj;
				player_left_obj["type"] = "lobby_player_left_game";
				player_left_obj["user"] = user;
				data_.post_message_to_game_clients(game_id, json_spirit::mValue(player_left_obj));
			}
			// Post message to all the other users that someone left.
			json_spirit::mObject mobj;
			mobj["type"] = "lobby_user_quit";
			mobj["user"] = user;
			data_.post_message_to_all_clients(json_spirit::mValue(mobj));

			// We specifically have to send the lobby_user_quit message back to the client here
			// as the sign_off function erases the client and the associated queue, etc.
			obj["type"] =  "lobby_user_quit";
			obj["user"] = user;
		} else {
			std::cerr << "lobby_quit failed for user: " << user << " : " << session_id << std::endl;
		}
		// We just return no content if session id and username not valid.
	} else if(type == "lobby_heartbeat") {
		return check_messages(user, rep, conn);
	} else if(type == "lobby_chat") {
		if(req_obj.find("message") == req_obj.end()) {
			obj["type"] = "error";
			obj["description"] = "'lobby_chat' message must have a 'message' attribute.";
		} else {
			if(req_obj.find("to") != req_obj.end()) {
				data_.post_message_to_client(req_obj["to"].get_str(), json_spirit::mValue(req_obj["message"]));
			} else {
				data_.post_message_to_all_clients(json_spirit::mValue(req_obj["message"]));
			}
			return check_messages(user, rep, conn);
		}
	} else if(type == "lobby_join_game") {
		if(req_obj.find("game_id") == req_obj.end()) {
			obj["type"] = "error";
			obj["description"] = "'lobby_join_game' message must have a 'game_id' attribute.";
		} else {
			json_spirit::mObject req_to_join;
			req_to_join["type"] = "lobby_request_to_join";
			req_to_join["user"] = user;
			data_.post_message_to_game_clients(req_obj["game_id"].get_int(), json_spirit::mValue(req_to_join));
			return check_messages(user, rep, conn);
		}
	} else if(type == "lobby_leave_game") {
		if(req_obj.find("game_id") != req_obj.end()) {
			if(data_.check_game_and_client(req_obj["game_id"].get_int(), user, "")) {
				if(!data_.check_client_in_games(user)) {
					// Still users in game post a message to them
					json_spirit::mObject player_left_obj;
					player_left_obj["type"] = "lobby_player_left_game";
					player_left_obj["user"] = user;
					data_.post_message_to_game_clients(req_obj["game_id"].get_int(), json_spirit::mValue(player_left_obj));
				}
				obj["type"] = "leave_game_success";
			} else {
				obj["type"] = "error";
				obj["description"] = "Invalid 'game_id' or you are not in that game.";
			}
		} else {
			obj["type"] = "error";
			obj["description"] = "'leave_game' message must have a 'game_id' attribute.";
		}
	} else if(type == "lobby_join_reply") {
		if(req_obj.find("game_id") != req_obj.end() && req_obj.find("accept") != req_obj.end() && req_obj.find("requesting_user") != req_obj.end()) {
			int game_id = req_obj["game_id"].get_int();
			if(data_.check_game_and_client(game_id, user, req_obj["requesting_user"].get_str())) {
				json_spirit::mObject join_reply_obj;
				join_reply_obj["type"] = "lobby_player_join_reply";
				join_reply_obj["requesting_user"] = req_obj["requesting_user"].get_str();
				join_reply_obj["accept"] = req_obj["accept"].get_bool();
				join_reply_obj["game_id"] = game_id;
				data_.post_message_to_game_clients(game_id, json_spirit::mValue(join_reply_obj));
				return check_messages(user, rep, conn);	
			} else {
				obj["type"] = "error";
				obj["description"] = "Invalid 'game_id' or you are not in that game.";
			}
		} else {
			obj["type"] = "error";
			obj["description"] = "'lobby_join_reply' message must have 'game_id', 'accept' and 'requesting_user' attributes.";
		}
	} else {
		obj["type"] = "error";
		obj["description"] = "Unknown type of request";
	}
	return reply::create_json_reply(json_spirit::mValue(obj), rep);
}

} // namespace server
} // namespace http
