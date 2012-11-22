#ifndef SERVER_HPP_INCLUDED
#define SERVER_HPP_INCLUDED

#include "tbs_game.hpp"
#include "variant.hpp"

#include <deque>
#include <map>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>

namespace tbs {

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
typedef boost::shared_ptr<boost::array<char, 1024> > buffer_ptr;

class server
{
public:
	explicit server(boost::asio::io_service& io_service);
	 
	void adopt_ajax_socket(socket_ptr socket, int session_id, const variant& msg);

	void clear_games();
private:

	struct game_info {
		game_info(const variant& value);
		game_ptr game_state;
		std::vector<int> clients;
	};

	typedef boost::shared_ptr<game_info> game_info_ptr;
	struct client_info {
		client_info();

		std::string user;	
		game_info_ptr game;
		int nplayer;
		int last_contact;

		int session_id;

		std::deque<std::string> msg_queue;
	};

	void handle_message_internal(socket_ptr socket, client_info& cli_info, const variant& msg);

	void close_ajax(socket_ptr socket, client_info& cli_info);

	void send_msg(socket_ptr socket, const variant& msg);
	void send_msg(socket_ptr socket, const char* msg);
	void send_msg(socket_ptr socket, const std::string& msg);
	void handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, boost::shared_ptr<std::string> buf);

	void disconnect(socket_ptr socket);

	void heartbeat();

	boost::asio::deadline_timer timer_;

	void quit_games(int session_id);

	void flush_game_messages(game_info& info);

	struct socket_info {
		std::vector<char> partial_message;
		std::string nick;
		int session_id;
	};

	void queue_msg(int session_id, const std::string& msg);

	variant create_lobby_msg() const;
	variant create_game_info_msg(game_info_ptr g) const;

	std::map<socket_ptr, std::string> waiting_connections_;

	std::map<socket_ptr, socket_info> connections_;
	std::map<int, client_info> clients_;
	std::vector<game_info_ptr> games_;

	//sockets waiting on status info.
	std::vector<socket_ptr> status_sockets_;

	int nheartbeat_;
	int scheduled_write_;
	void schedule_write();

	void status_change();
	int status_id_;
};

}

#endif
