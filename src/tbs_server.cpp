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

server::game_info::game_info(const variant& value)
{
	game_state = game::create(value);
}

server::client_info::client_info() : nplayer(0), last_contact(0)
{}

server::server(boost::asio::io_service& io_service)
  : server_base(io_service)
{
}

server::~server()
{
}

void server::adopt_ajax_socket(socket_ptr socket, int session_id, const variant& msg)
{
	handle_message(
		boost::bind(static_cast<void(server::*)(socket_ptr,const variant&)>(&server::send_msg), this, socket, _1), 
		boost::bind(&server::close_ajax, this, socket, _1),
		boost::bind(&server::get_socket_info, this, socket),
		session_id, 
		msg);
}

server_base::socket_info& server::get_socket_info(socket_ptr socket)
{
	return connections_[socket];
}

void server::close_ajax(socket_ptr socket, client_info& cli_info)
{
	socket_info& info = connections_[socket];
	ASSERT_LOG(info.session_id != -1, "UNKNOWN SOCKET");

	if(cli_info.msg_queue.empty() == false) {
		std::vector<socket_ptr> keepalive_sockets;

		for(std::map<socket_ptr,std::string>::iterator s = waiting_connections_.begin();
		    s != waiting_connections_.end(); ) {
			if(s->second == info.nick && s->first != socket) {
				keepalive_sockets.push_back(s->first);
				sessions_to_waiting_connections_.erase(cli_info.session_id);
				waiting_connections_.erase(s++);
			} else {
				++s;
			}
		}

		const std::string msg = cli_info.msg_queue.front();
		cli_info.msg_queue.pop_front();
		send_msg(socket, msg);

		foreach(socket_ptr socket, keepalive_sockets) {
			send_msg(socket, "{ \"type\": \"keepalive\" }");
		}
	} else {
		waiting_connections_[socket] = info.nick;
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
		sessions_to_waiting_connections_.erase(session_id);
		send_msg(sock, msg);
		return;
	}

	server_base::queue_msg(session_id, msg, has_priority);
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
	std::map<socket_ptr, socket_info>::const_iterator connections_itor = connections_.find(socket);
	const int session_id = connections_itor == connections_.end() ? -1 : connections_itor->second.session_id;

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
	boost::asio::async_write(*socket, boost::asio::buffer(*str_buf),
			                         boost::bind(&server::handle_send, this, socket, _1, _2, str_buf, session_id));
}

void server::handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, boost::shared_ptr<std::string> buf, int session_id)
{
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

	std::map<socket_ptr, socket_info>::iterator itor = connections_.find(socket);
	if(itor != connections_.end()) {
		std::map<int, socket_ptr>::iterator sessions_itor = sessions_to_waiting_connections_.find(itor->second.session_id);
		if(sessions_itor != sessions_to_waiting_connections_.end() && sessions_itor->second == socket) {
			sessions_to_waiting_connections_.erase(sessions_itor);
		}

		connections_.erase(itor);
	}

	waiting_connections_.erase(socket);

	socket->close();
}

void server::heartbeat_internal(int send_heartbeat, std::map<int, client_info>& clients)
{
	std::vector<std::pair<socket_ptr, std::string> > messages;

	for(std::map<socket_ptr, std::string>::iterator i = waiting_connections_.begin(); i != waiting_connections_.end(); ++i) {
		socket_ptr socket = i->first;

		socket_info& info = connections_[socket];
		ASSERT_LOG(info.session_id != -1, "UNKNOWN SOCKET");

		client_info& cli_info = clients[info.session_id];
		if(cli_info.msg_queue.empty() == false) {
			messages.push_back(std::pair<socket_ptr,std::string>(socket, cli_info.msg_queue.front()));
			cli_info.msg_queue.pop_front();

		sessions_to_waiting_connections_.erase(info.session_id);
		} else if(send_heartbeat) {
			if(!cli_info.game) {
				messages.push_back(std::pair<socket_ptr,std::string>(socket, "{ \"type\": \"heartbeat\" }"));
			} else {
				variant v = create_heartbeat_packet(cli_info);
				messages.push_back(std::pair<socket_ptr,std::string>(socket, v.write_json()));
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
}

}
