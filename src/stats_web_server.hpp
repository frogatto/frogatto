#ifndef WEB_SERVER_HPP_INCLUDED
#define WEB_SERVER_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/asio.hpp>

class web_server
{
public:
	typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
	typedef boost::shared_ptr<boost::array<char, 1024> > buffer_ptr;

	explicit web_server(boost::asio::io_service& io_service, int port=23456);
private:

	void start_accept();
	void handle_accept(socket_ptr socket, const boost::system::error_code& error);

	struct receive_buf {
		receive_buf() : wanted(-1) {}
		std::string msg;
		int wanted;
	};

	typedef boost::shared_ptr<receive_buf> receive_buf_ptr;

	void start_receive(socket_ptr socket, receive_buf_ptr buf=receive_buf_ptr());
	void handle_receive(socket_ptr socket, buffer_ptr buf, const boost::system::error_code& e, size_t nbytes, receive_buf_ptr recv_buf);
	void handle_incoming_data(socket_ptr socket, const char* i1, const char* i2, receive_buf_ptr recv_buf);

	void handle_message(socket_ptr socket, receive_buf_ptr recv_buf);

	void send_msg(socket_ptr socket, const std::string& type, const std::string& msg, const std::string& header_parms);
	void send_404(socket_ptr socket);

	void handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, size_t max_bytes, boost::shared_ptr<std::string> buf);

	void disconnect(socket_ptr socket);

	void heartbeat();

	boost::asio::ip::tcp::acceptor acceptor_;

	boost::asio::deadline_timer timer_;
	int nheartbeat_;
};

#endif
