#include <assert.h>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <string>
#include <vector>

#include <iostream>
#include <boost/cstdint.hpp>

#include "asserts.hpp"
#include "foreach.hpp"

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

typedef boost::shared_ptr<tcp::socket> socket_ptr;
typedef boost::shared_ptr<boost::array<char, 1024> > buffer_ptr;

class server
{
public:
	explicit server(boost::asio::io_service& io_service)
	  : acceptor_(io_service, tcp::endpoint(tcp::v4(), 17002)),
	    next_id_(0),
		udp_socket_(io_service, udp::endpoint(udp::v4(), 17001))
	{
		start_accept();
		start_udp_receive();
	}

private:
	void start_accept()
	{
		socket_ptr socket(new tcp::socket(acceptor_.get_io_service()));
		acceptor_.async_accept(*socket, boost::bind(&server::handle_accept, this, socket, boost::asio::placeholders::error));
	}

	void handle_accept(socket_ptr socket, const boost::system::error_code& error)
	{
		if(!error) {
			SocketInfo info;
			info.id = next_id_++;

			boost::array<char, 4> buf;
			memcpy(&buf[0], &info.id, 4);

			boost::asio::async_write(*socket, boost::asio::buffer(buf),
			                         boost::bind(&server::handle_send, this, socket, _1, _2));

			sockets_info_[socket] = info;
			id_to_socket_[info.id] = socket;

			sockets_.push_back(socket);
			start_receive(socket);
			start_accept();
		} else {
			std::cerr << "ERROR IN ACCEPT!\n";
		}
	}

	void handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes)
	{
		if(e) {
			disconnect(socket);
		}
	}

	void start_receive(socket_ptr socket)
	{
		buffer_ptr buf(new boost::array<char, 1024>);
		socket->async_read_some(boost::asio::buffer(*buf), boost::bind(&server::handle_receive, this, socket, buf, _1, _2));
	}

	void handle_receive(socket_ptr socket, buffer_ptr buf, const boost::system::error_code& e, size_t nbytes)
	{
		if(!e) {
			std::string str(buf->data(), buf->data() + nbytes);
			std::cerr << "RECEIVE {{{" << str << "}}}\n";

			static boost::regex ready("READY/(.+)/(\\d+)/(.* \\d+)");

			boost::cmatch match;
			if(boost::regex_match(str.c_str(), match, ready)) {
				std::string level_id(match[1].first, match[1].second);

				GameInfoPtr& game = games_[level_id];
				if(!game) {
					game.reset(new GameInfo);
				}

				SocketInfo& info = sockets_info_[socket];
				info.local_addr = std::string(match[3].first, match[3].second);
				if(info.game) {
					//if the player is already in a game, remove them from it.
					std::vector<socket_ptr>& v = info.game->players;
					v.erase(std::remove(v.begin(), v.end(), socket), v.end());
				}

				info.game = game;

				fprintf(stderr, "ADDING PLAYER TO GAME: %p\n", socket.get());
				game->players.push_back(socket);


				const size_t nplayers = atoi(match[2].first);
				game->nplayers = nplayers;

			}

			start_receive(socket);
		} else {
			disconnect(socket);
		}
	}

	void disconnect(socket_ptr socket) {

		SocketInfo& info = sockets_info_[socket];
		if(info.game) {
			std::vector<socket_ptr>& v = info.game->players;
			v.erase(std::remove(v.begin(), v.end(), socket), v.end());
		}

		std::cerr << "CLOSING SOCKET: ";
		socket->close();
		id_to_socket_.erase(sockets_info_[socket].id);
		sockets_.erase(std::remove(sockets_.begin(), sockets_.end(), socket), sockets_.end());

		sockets_info_.erase(socket);
	}

	tcp::acceptor acceptor_;

	std::vector<socket_ptr> sockets_;

	typedef boost::shared_ptr<udp::endpoint> udp_endpoint_ptr;

	struct GameInfo;
	typedef boost::shared_ptr<GameInfo> GameInfoPtr;

	struct SocketInfo {
		uint32_t id;
		udp_endpoint_ptr udp_endpoint;
		GameInfoPtr game;
		std::string local_addr;
	};

	std::map<socket_ptr, SocketInfo> sockets_info_;
	std::map<uint32_t, socket_ptr> id_to_socket_;

	struct GameInfo {
		GameInfo() : started(false) {}
		std::vector<socket_ptr> players;
		size_t nplayers;
		bool started;
	};

	std::map<std::string, GameInfoPtr> games_;

	uint32_t next_id_;

	void start_udp_receive() {
		udp_endpoint_ptr endpoint(new udp::endpoint);
		udp_socket_.async_receive_from(
		  boost::asio::buffer(udp_buf_), *endpoint,
		  boost::bind(&server::handle_udp_receive, this, endpoint, _1, _2));
	}

	void handle_udp_receive(udp_endpoint_ptr endpoint, const boost::system::error_code& error, size_t len)
	{
		fprintf(stderr, "RECEIVED UDP PACKET: %d\n", len);
		if(len >= 5) {
			uint32_t id;
			memcpy(&id, &udp_buf_[1], 4);
			std::map<uint32_t, socket_ptr>::iterator socket_it = id_to_socket_.find(id);
			if(socket_it != id_to_socket_.end()) {
				assert(sockets_info_.count(socket_it->second));
				sockets_info_[socket_it->second].udp_endpoint = endpoint;

				GameInfoPtr& game = sockets_info_[socket_it->second].game;
				if(udp_buf_[0] == 'Z' && game.get() != NULL && !game->started && game->players.size() >= game->nplayers) {
					bool have_sockets = true;
					foreach(const socket_ptr& sock, game->players) {
						const SocketInfo& info = sockets_info_[sock];
						if(info.udp_endpoint.get() == NULL) {
							have_sockets = false;
						}
					}

					if(have_sockets) {

						foreach(socket_ptr socket, game->players) {
							const SocketInfo& send_socket_info = sockets_info_[socket];

							std::ostringstream msg;
							msg << "START " << game->players.size() << "\n";
							foreach(socket_ptr s, game->players) {
								if(s == socket) {
									msg << "SLOT\n";
									continue;
								}

								const SocketInfo& sock_info = sockets_info_[s];
								if(send_socket_info.udp_endpoint->address().to_string() != sock_info.udp_endpoint->address().to_string()) {
									//the hosts are not from the same address,
									//so send them each other's network address.
									msg << sock_info.udp_endpoint->address().to_string().c_str() << " " << sock_info.udp_endpoint->port() << "\n";
								} else {
									//the hosts are from the same address,
									//which means they are likely behind the
									//same NAT device. Send them their local
									//addresses, behind their devices.
									msg << sock_info.local_addr << "\n";
								}
							}

							const std::string msg_str = msg.str();

							boost::asio::async_write(*socket, boost::asio::buffer(msg_str),
				                         boost::bind(&server::handle_send, this, socket, _1, _2));
						}

						game->started = true;
						for(std::map<std::string, GameInfoPtr>::iterator i = games_.begin(); i != games_.end(); ++i) {
							if(game == i->second) {
								i->second.reset();
							}
						}
					}
				}

				if(udp_buf_[0] != 'Z') {
					GameInfoPtr game = sockets_info_[socket_it->second].game;
	
					if(game.get() != NULL) {
						for(int n = 0; n != game->players.size(); ++n) {
							if(game->players[n] == socket_it->second) {
								continue;
							}

							std::cerr << "GOT FROM: " << endpoint->port() << " RELAYING TO...\n";
							std::map<socket_ptr, SocketInfo>::iterator socket_info = sockets_info_.find(game->players[n]);
							if(socket_info != sockets_info_.end()) {
								std::cerr << "  RELAY TO " << socket_info->second.udp_endpoint->port() << "\n";
								udp_socket_.async_send_to(boost::asio::buffer(&udp_buf_[0], len), *socket_info->second.udp_endpoint,
								    boost::bind(&server::handle_udp_send, this, socket_info->second.udp_endpoint, _1, _2));
							}
						}
					}
				}
			}
		}
		start_udp_receive();
	}

	void handle_udp_send(udp_endpoint_ptr endpoint, const boost::system::error_code& error, size_t len)
	{
		if(error) {
			fprintf(stderr, "ERROR IN UDP SEND!\n");
		} else {
			fprintf(stderr, "UDP: SENT %d BYTES\n", (int)len);
		}
	}

	udp::socket udp_socket_;
	boost::array<char, 1024> udp_buf_;
};

int main(int argc, char** argv)
{
	boost::asio::io_service io_service;

	server srv(io_service);
	io_service.run();
}
