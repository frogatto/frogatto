#include <sstream>
#include <string>

#include <inttypes.h>
#include <numeric>
#include <stdio.h>
#include <unistd.h>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

#include "asserts.hpp"
#include "controls.hpp"
#include "level.hpp"
#include "multiplayer.hpp"
#include "unit_test.hpp"

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace multiplayer {

namespace {
boost::shared_ptr<boost::asio::io_service> asio_service;
boost::shared_ptr<tcp::socket> tcp_socket;
boost::shared_ptr<udp::socket> udp_socket;
boost::shared_ptr<udp::endpoint> udp_endpoint;

std::vector<boost::shared_ptr<udp::endpoint> > udp_endpoint_peers;

int32_t id;
int player_slot;

bool udp_packet_waiting()
{
	if(!udp_socket) {
		return false;
	}

	boost::asio::socket_base::bytes_readable command(true);
	udp_socket->io_control(command);
	return command.get() != 0;
}

bool tcp_packet_waiting()
{
	if(!tcp_socket) {
		return false;
	}

	boost::asio::socket_base::bytes_readable command(true);
	tcp_socket->io_control(command);
	return command.get() != 0;
}
}

int slot()
{
	return player_slot;
}

manager::manager(bool activate)
{
	if(activate) {
		asio_service.reset(new boost::asio::io_service);
	}
}

manager::~manager() {
	udp_endpoint.reset();
	tcp_socket.reset();
	udp_socket.reset();
	asio_service.reset();
	player_slot = 0;
}

void setup_networked_game(const std::string& server)
{
	boost::asio::io_service& io_service = *asio_service;
	tcp::resolver resolver(io_service);

	tcp::resolver::query query(server, "17002");

	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;

	tcp_socket.reset(new tcp::socket(io_service));
	tcp::socket& socket = *tcp_socket;
	boost::system::error_code error = boost::asio::error::host_not_found;
	while(error && endpoint_iterator != end) {
		socket.close();
		socket.connect(*endpoint_iterator++, error);
	}

	if(error) {
		fprintf(stderr, "NETWORK ERROR: Can't resolve host\n");
		throw multiplayer::error();
	}

	boost::array<char, 5> initial_response;
	size_t len = socket.read_some(boost::asio::buffer(initial_response), error);
	if(error) {
		fprintf(stderr, "ERROR READING INITIAL RESPONSE\n");
		throw multiplayer::error();
	}

	if(len != 5) {
		fprintf(stderr, "INITIAL RESPONSE HAS THE WRONG SIZE: %d\n", (int)len);
		throw multiplayer::error();
	}

	memcpy(&id, &initial_response[0], 4);
	player_slot = initial_response[4];

	fprintf(stderr, "ID: %d; SLOT: %d\n", id, player_slot);

    udp::resolver udp_resolver(io_service);
    udp::resolver::query udp_query(udp::v4(), server, "17001");
	udp_endpoint.reset(new udp::endpoint);
    *udp_endpoint = *udp_resolver.resolve(udp_query);
    udp::endpoint& receiver_endpoint = *udp_endpoint;

	udp_socket.reset(new udp::socket(io_service));
    udp_socket->open(udp::v4());

	boost::array<char, 4> udp_msg;
	memcpy(&udp_msg[0], &id, 4);

//	udp_socket->send_to(boost::asio::buffer(udp_msg), receiver_endpoint);

	fprintf(stderr, "SENT UDP PACKET\n");

	udp::endpoint sender_endpoint;
//	len = udp_socket->receive_from(boost::asio::buffer(udp_msg), sender_endpoint);
	fprintf(stderr, "GOT UDP PACKET: %d\n", (int)len);

	std::string msg = "greetings!";
	socket.write_some(boost::asio::buffer(msg), error);
	if(error) {
		fprintf(stderr, "NETWORK ERROR: Could not send data\n");
		throw multiplayer::error();
	}
}

void sync_start_time(const level& lvl, boost::function<bool()> idle_fn)
{
	if(!tcp_socket) {
		return;
	}

	std::ostringstream s;
	s << "READY/" << lvl.id() << "/" << lvl.players().size();
	boost::system::error_code error = boost::asio::error::host_not_found;
	tcp_socket->write_some(boost::asio::buffer(s.str()), error);
	if(error) {
		fprintf(stderr, "ERROR WRITING TO SOCKET\n");
		throw multiplayer::error();
	}

	while(!tcp_packet_waiting()) {
		if(idle_fn) {
			const bool res = idle_fn();
			if(!res) {
				std::cerr << "quitting game...\n";
				throw multiplayer::error();
			}
		}

		std::vector<char> send_buf(4);
		memcpy(&send_buf[0], &id, 4);
		udp_socket->send_to(boost::asio::buffer(send_buf), *udp_endpoint);
	}

	boost::array<char, 1024> response;
	size_t len = tcp_socket->read_some(boost::asio::buffer(response), error);
	if(error) {
		fprintf(stderr, "ERROR READING FROM SOCKET\n");
		throw multiplayer::error();
	}

	std::string str(&response[0], &response[0] + len);
	if(std::string(str.begin(), str.begin() + 5) != "START") {
		fprintf(stderr, "UNEXPECTED RESPONSE: '%s'\n", str.c_str());
		throw multiplayer::error();
	}

	const char* ptr = str.c_str() + 6;
	char* end_ptr = NULL;
	const int nplayers = strtol(ptr, &end_ptr, 10);
	ptr = end_ptr;
	ASSERT_EQ(*ptr, '\n');
	++ptr;

	boost::asio::io_service& io_service = *asio_service;
    udp::resolver udp_resolver(io_service);

	udp_endpoint_peers.clear();

	for(int n = 0; n != nplayers; ++n) {
		const char* end = strchr(ptr, ' ');
		ASSERT_LOG(end != NULL, "ERROR PARSING RESPONSE: " << str);
		std::string host(ptr, end);
		ptr = end+1;
		end = strchr(ptr, '\n');
		ASSERT_LOG(end != NULL, "ERROR PARSING RESPONSE: " << str);
		std::string port(ptr, end);
		ptr = end+1;

		udp::resolver::query peer_query(udp::v4(), host, port);

		if(n != player_slot) {
			udp_endpoint_peers.push_back(boost::shared_ptr<udp::endpoint>(new udp::endpoint));
			*udp_endpoint_peers.back() = *udp_resolver.resolve(peer_query);
		} else {
			//this is ourself, don't record our endpoint.
			udp_endpoint_peers.push_back(boost::shared_ptr<udp::endpoint>());
		}
	}

	std::set<int> confirmed_players;
	confirmed_players.insert(player_slot);

	std::cerr << "PLAYER " << player_slot << " CONFIRMING...\n";

	for(int m = 0; m != 10000 && confirmed_players.size() < nplayers || m < 500; ++m) {
		boost::array<char, 4096> udp_msg;
		std::string msg = "AA";
		msg[1] = player_slot;
		for(int n = 0; n != nplayers; ++n) {
			if(n == player_slot) {
				continue;
			}

			std::cerr << "SENDING CONFIRM TO " << udp_endpoint_peers[n]->port() << "\n";
			udp_socket->send_to(boost::asio::buffer(msg), *udp_endpoint_peers[n]);
		}

		while(udp_packet_waiting()) {
			size_t len = udp_socket->receive(boost::asio::buffer(udp_msg));
			if(len == 2 && udp_msg[0] == 'A') {
				confirmed_players.insert(udp_msg[1]);
				std::cerr << "CONFIRMED PLAYER: " << static_cast<int>(udp_msg[1]) << "/ " << player_slot << "\n";
			}
		}

		SDL_Delay(1);
	}

	if(confirmed_players.size() < nplayers) {
		std::cerr << "COULD NOT CONFIRM NETWORK CONNECTION TO ALL PEERS\n";
		throw multiplayer::error();
	}

	controls::set_delay(3);

	if(player_slot == 0) {
		int ping_id = 0;
		std::map<int, int> ping_sent_at;
		std::map<int, int> ping_player;
		std::map<std::string, int> contents_ping;

		std::map<int, int> player_nresponses;
		std::map<int, int> player_latency;

		const int game_start = SDL_GetTicks() + 1000;
		boost::array<char, 1024> receive_buf;
		while(SDL_GetTicks() < game_start) {
			const int ticks = SDL_GetTicks();
			const int start_in = game_start - ticks;

			for(int n = 0; n != nplayers; ++n) {
				if(n == player_slot) {
					continue;
				}

				char buf[128];

				int start_advisory = start_in;
				const int player_responses = player_nresponses[n];
				if(player_responses) {
					const int avg_latency = player_latency[n]/player_responses;
					start_advisory -= avg_latency/2;
					if(start_advisory < 0) {
						start_advisory = 0;
					}
				}

				fprintf(stderr, "SENDING ADVISORY TO START IN %d - %d\n", start_in, (start_in - start_advisory));

				sprintf(buf, "P%d %d", ping_id, start_advisory);

				std::string msg(buf);
				udp_socket->send_to(boost::asio::buffer(msg), *udp_endpoint_peers[n]);
				ping_sent_at[ping_id] = ticks;
				contents_ping[msg] = ping_id;
				ping_player[ping_id] = n;
				ping_id++;
			}

			while(udp_packet_waiting()) {
				size_t len = udp_socket->receive(boost::asio::buffer(receive_buf));
				if(len > 1 && receive_buf[0] == 'P') {
					std::string msg(&receive_buf[0], &receive_buf[0] + len);
					ASSERT_LOG(contents_ping.count(msg), "UNRECOGNIZED PING: " << msg);
					const int ping = contents_ping[msg];
					const int latency = ticks - ping_sent_at[ping];
					const int nplayer = ping_player[ping];

					player_nresponses[nplayer]++;
					player_latency[nplayer] += latency;

					fprintf(stderr, "RECEIVED PING FROM %d IN %d AVG LATENCY %d\n", nplayer, latency, player_latency[nplayer]/player_nresponses[nplayer]);
				}
			}
			
			SDL_Delay(1);
		}
	} else {
		std::vector<int> start_time;
		boost::array<char, 1024> buf;
		for(;;) {
			while(udp_packet_waiting()) {
				size_t len = udp_socket->receive(boost::asio::buffer(buf));
				if(len > 1 && buf[0] == 'P') {
					const std::string s(&buf[0], &buf[0] + len);

					std::string::const_iterator i = std::find(s.begin(), s.end(), ' ');
					ASSERT_LOG(i != s.end(), "NO WHITE SPACE FOUND IN PING MESSAGE: " << s);
					const std::string start_in(i+1, s.end());
					const int start_in_num = atoi(start_in.c_str());
					start_time.push_back(SDL_GetTicks() + start_in_num);
					while(start_time.size() > 5) {
						start_time.erase(start_time.begin());
					}
					udp_socket->send_to(boost::asio::buffer(s), *udp_endpoint_peers[0]);
				}
			}

			if(start_time.size() > 0) {
				const int start_time_avg = std::accumulate(start_time.begin(), start_time.end(), 0)/start_time.size();
				if(SDL_GetTicks() >= start_time_avg) {
					break;
				}
			}

			SDL_Delay(1);
		}
	}
}

void send_and_receive()
{
	if(!udp_socket || controls::num_players() == 1) {
		return;
	}

	//send our ID followed by the send packet.
	std::vector<char> send_buf(5);
	send_buf[0] = 'C';
	memcpy(&send_buf[1], &id, 4);
	controls::write_control_packet(send_buf);

	for(int n = 0; n != udp_endpoint_peers.size(); ++n) {
		if(n == player_slot) {
			continue;
		}

		udp_socket->send_to(boost::asio::buffer(send_buf), *udp_endpoint_peers[n]);
	}

	while(udp_packet_waiting()) {
		udp::endpoint sender_endpoint;
		boost::array<char, 4096> udp_msg;
		size_t len = udp_socket->receive(boost::asio::buffer(udp_msg));
		if(len == 0 || udp_msg[0] != 'C') {
			continue;
		}

		if(len < 5) {
			fprintf(stderr, "UDP PACKET TOO SHORT: %d\n", (int)len);
			continue;
		}

		controls::read_control_packet(&udp_msg[5], len - 5);
	}
}

}

namespace {
struct Peer {
	std::string host, port;
};
}

UTILITY(hole_punch_test) {
	boost::asio::io_service io_service;
	udp::resolver udp_resolver(io_service);

	const char* server_hostname = "wesnoth.org";
	const char* server_port = "17001";

	udp::resolver::query udp_query(udp::v4(), server_hostname, server_port);
	udp::endpoint udp_endpoint;
	udp_endpoint = *udp_resolver.resolve(udp_query);

	udp::socket udp_socket(io_service);
	udp_socket.open(udp::v4());

	udp_socket.send_to(boost::asio::buffer("hello"), udp_endpoint);

	std::vector<Peer> peers;

	boost::array<char, 1024> buf;
	for(;;) {
		udp_socket.receive(boost::asio::buffer(buf));
		fprintf(stderr, "RECEIVED {{{%s}}}\n", &buf[0]);

		char* beg = &buf[0];
		char* mid = strchr(beg, ' ');
		if(mid) {
			*mid = 0;
			const char* port = mid+1;

			Peer peer;
			peer.host = beg;
			peer.port = port;
	
			peers.push_back(peer);
		}

		for(int m = 0; m != 10; ++m) {
			for(int n = 0; n != peers.size(); ++n) {
				const std::string host = peers[n].host;
				const std::string port = peers[n].port;
				fprintf(stderr, "sending to %s %s\n", host.c_str(), port.c_str());
				udp::resolver::query peer_query(udp::v4(), host, port);
				udp::endpoint peer_endpoint;
				peer_endpoint = *udp_resolver.resolve(peer_query);

				udp_socket.send_to(boost::asio::buffer("peer"), peer_endpoint);
			}

			sleep(1);
		}
	}

	io_service.run();
}
