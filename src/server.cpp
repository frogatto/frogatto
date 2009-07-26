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
#include <inttypes.h>

#include "foreach.hpp"

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

typedef boost::shared_ptr<tcp::socket> socket_ptr;
typedef boost::shared_ptr<boost::array<char, 1024> > buffer_ptr;

class server
{
public:
	explicit server(boost::asio::io_service& io_service)
	  : acceptor_(io_service, tcp::endpoint(tcp::v4(), 17000)),
	    next_id_(0), slots_(0),
		udp_socket_(io_service, udp::endpoint(udp::v4(), 17001)),
		nready_(0)
	{
		start_accept();
		start_udp_receive();
	}

private:
	void start_accept()
	{
		socket_ptr socket(new tcp::socket(acceptor_.io_service()));
		acceptor_.async_accept(*socket, boost::bind(&server::handle_accept, this, socket, boost::asio::placeholders::error));
	}

	void handle_accept(socket_ptr socket, const boost::system::error_code& error)
	{
		if(!error) {
			SocketInfo info;
			info.id = next_id_++;
			info.slot = 0;
			while(slots_&(1 << info.slot)) {
				++info.slot;
			}

			slots_ |= (1 << info.slot);

			boost::array<char, 5> buf;
			memcpy(&buf[0], &info.id, 4);
			buf[4] = info.slot;

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

			static boost::regex ready("READY/(\\d+)");

			boost::cmatch match;
			if(boost::regex_match(str.c_str(), match, ready)) {
				++nready_;
				const int nplayers = atoi(match[1].first);
				if(nready_ >= nplayers) {
					std::string msg = "START";
					foreach(socket_ptr socket, sockets_) {
						boost::asio::async_write(*socket, boost::asio::buffer(msg),
			                         boost::bind(&server::handle_send, this, socket, _1, _2));
					}
					nready_ = 0;
				}
			}

			start_receive(socket);
		} else {
			disconnect(socket);
		}
	}

	void disconnect(socket_ptr socket) {

		std::cerr << "CLOSING SOCKET: ";
		socket->close();
		id_to_socket_.erase(sockets_info_[socket].id);
		sockets_.erase(std::remove(sockets_.begin(), sockets_.end(), socket), sockets_.end());

		std::cerr << sockets_info_[socket].id << " " << slots_ << " -> ";
		slots_ &= ~(1LL << sockets_info_[socket].slot);
		std::cerr << slots_ << "\n";
		sockets_info_.erase(socket);
	}

	tcp::acceptor acceptor_;

	std::vector<socket_ptr> sockets_;

	typedef boost::shared_ptr<udp::endpoint> udp_endpoint_ptr;

	struct SocketInfo {
		uint32_t id;
		uint8_t slot;
		udp_endpoint_ptr udp_endpoint;
	};

	std::map<socket_ptr, SocketInfo> sockets_info_;
	std::map<uint32_t, socket_ptr> id_to_socket_;

	uint32_t next_id_;
	uint64_t slots_;

	void start_udp_receive() {
		udp_endpoint_ptr endpoint(new udp::endpoint);
		udp_socket_.async_receive_from(
		  boost::asio::buffer(udp_buf_), *endpoint,
		  boost::bind(&server::handle_udp_receive, this, endpoint, _1, _2));
	}

	void handle_udp_receive(udp_endpoint_ptr endpoint, const boost::system::error_code& error, size_t len)
	{
		fprintf(stderr, "RECEIVED UDP PACKET: %d\n", len);
		if(len >= 4) {
			uint32_t id;
			memcpy(&id, &udp_buf_[0], 4);
			std::map<uint32_t, socket_ptr>::iterator socket_it = id_to_socket_.find(id);
			if(socket_it != id_to_socket_.end()) {
				assert(sockets_info_.count(socket_it->second));
				sockets_info_[socket_it->second].udp_endpoint = endpoint;

				for(std::map<socket_ptr, SocketInfo>::iterator i = sockets_info_.begin(); i != sockets_info_.end(); ++i) {
					if(i->first != socket_it->second && i->second.udp_endpoint) {
						udp_socket_.async_send_to(boost::asio::buffer(&udp_buf_[0], len), *i->second.udp_endpoint,
						    boost::bind(&server::handle_udp_send, this, i->second.udp_endpoint, _1, _2));
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
	int nready_;
};

int main(int argc, char** argv)
{
	boost::asio::io_service io_service;

	server srv(io_service);
	io_service.run();
}
