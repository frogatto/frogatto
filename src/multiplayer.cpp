#include <sstream>
#include <string>

#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

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

	tcp::resolver::query query(server, "17000");

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
	}

	boost::array<char, 1024> response;
	size_t len = tcp_socket->read_some(boost::asio::buffer(response), error);
	if(error) {
		fprintf(stderr, "ERROR READING FROM SOCKET\n");
		throw multiplayer::error();
	}

	std::string str(&response[0], &response[0] + len);
	if(str != "START") {
		fprintf(stderr, "UNEXPECTED RESPONSE: '%s'\n", str.c_str());
		throw multiplayer::error();
	}
}

void send_and_receive()
{
	if(!udp_socket || controls::num_players() == 1) {
		return;
	}

	//send our ID followed by the send packet.
	std::vector<char> send_buf(4);
	memcpy(&send_buf[0], &id, 4);
	controls::write_control_packet(send_buf);
	udp_socket->send_to(boost::asio::buffer(send_buf), *udp_endpoint);

	while(udp_packet_waiting()) {
		udp::endpoint sender_endpoint;
		boost::array<char, 4096> udp_msg;
		size_t len = udp_socket->receive_from(boost::asio::buffer(udp_msg), sender_endpoint);
		if(len < 4) {
			fprintf(stderr, "UDP PACKET TOO SHORT: %d\n", (int)len);
			continue;
		}

		controls::read_control_packet(&udp_msg[4], len - 4);
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
