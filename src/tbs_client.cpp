#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "asserts.hpp"
#include "tbs_client.hpp"

namespace tbs {

client::client(const std::string& host, const std::string& port, int session)
  : session_id_(session),
    resolver_(io_service_),
	host_(host),
	resolver_query_(host.c_str(), port.c_str())
{
}

void client::send_request(const std::string& request, game_logic::map_formula_callable_ptr callable, boost::function<void(std::string)> handler)
{
	connection_ptr conn(new Connection(io_service_));
	conn->request = request;
	conn->handler = handler;
	conn->callable = callable;

	resolver_.async_resolve(resolver_query_,
		boost::bind(&client::handle_resolve, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::iterator,
			conn));
}

void client::handle_resolve(const boost::system::error_code& error, tcp::resolver::iterator endpoint_iterator, connection_ptr conn)
{
	if(!error)
	{
		endpoint_iterator_ = endpoint_iterator;
		// Attempt a connection to each endpoint in the list until we
		// successfully establish a connection.
#if BOOST_VERSION >= 104700
		boost::asio::async_connect(conn->socket, 
			endpoint_iterator_,
			boost::bind(&client::handle_connect, this,
				boost::asio::placeholders::error, conn, endpoint_iterator_));
#else
		conn->socket.async_connect(*endpoint_iterator_,
			boost::bind(&client::handle_connect, this,
				boost::asio::placeholders::error, conn, endpoint_iterator_));
#endif
	} else {
		conn->callable->add("error", variant(error.message()));
		conn->handler("connection_error");
	}
}

void client::handle_connect(const boost::system::error_code& error, connection_ptr conn, tcp::resolver::iterator resolve_itor)
{
	if(error) {
		std::cerr << "HANDLE_CONNECT_ERROR: " << error << std::endl;
		if(endpoint_iterator_ == resolve_itor) {
			++endpoint_iterator_;
		}
		//ASSERT_LOG(endpoint_iterator_ != tcp::resolver::iterator(), "COULD NOT RESOLVE TBS SERVER: " << resolve_itor->endpoint().address().to_string() << ":" << resolve_itor->endpoint().port());
		if(endpoint_iterator_ == tcp::resolver::iterator()) {
			conn->callable->add("error", variant("Could not resolve TBS server."));
			conn->handler("connection_error");
			return;
		}

#if BOOST_VERSION >= 104700
		boost::asio::async_connect(conn->socket, 
			endpoint_iterator_,
			boost::bind(&client::handle_connect, this,
				boost::asio::placeholders::error, conn, endpoint_iterator_));
#else
		conn->socket.async_connect(*endpoint_iterator_,
 	       boost::bind(&client::handle_connect, this,
	          boost::asio::placeholders::error, conn, endpoint_iterator_));
#endif
		return;
	}

	// Indicate a successful connection.
	conn->handler("connection_success");

	//do async write.
	std::ostringstream msg;
	msg << "POST /tbs HTTP/1.1\r\n"
		   "Host: " << host_ << "\r\n"
		   "Accept: */*\r\n"
	       "User-Agent: Frogatto 1.0\r\n"
		   "Content-Type: text/plain\r\n"
		   "Connection: close\r\n";
	
	if(session_id_ != -1) {
		msg << "Cookie: session=" << session_id_ << "\r\n";
	}
	// replace all the tab characters with spaces before calculating the request size, so 
	// some http server doesn't get all upset about the length being wrong.
	boost::replace_all(conn->request, "\t", "    ");
	msg << "Content-Length: " << conn->request.length() << "\r\n\r\n" << conn->request;

	const std::string msg_str = msg.str();
	boost::asio::async_write(conn->socket, boost::asio::buffer(msg_str),
	      boost::bind(&client::handle_send, this, conn, _1, _2));
}

void client::handle_send(connection_ptr conn, const boost::system::error_code& e, size_t nbytes)
{
	ASSERT_LOG(!e, "ERROR SENDING DATA");

	conn->socket.async_read_some(boost::asio::buffer(conn->buf), boost::bind(&client::handle_receive, this, conn, _1, _2));
}

void client::handle_receive(connection_ptr conn, const boost::system::error_code& e, size_t nbytes)
{
	if(e.value() == 2) {
		// EOF
		std::cerr << "handle_receive EOF: " << nbytes << std::endl;
		return;
	}
	//std::cerr << "handle_receive error: " << e.message() << " : " << e.value() << " : " << nbytes << std::endl;
	ASSERT_LOG(e.value() == 0, "ERROR RECEIVING DATA");
	conn->response.insert(conn->response.end(), &conn->buf[0], &conn->buf[0] + nbytes);
	if(conn->expected_len == -1) {
		int header_term_len = 2;
		const char* end_headers = strstr(conn->response.c_str(), "\n\n");
		if(!end_headers) {
			end_headers = strstr(conn->response.c_str(), "\r\n\r\n");
			header_term_len = 4;
		}
		if(end_headers) {
			const char* content_length = strstr(conn->response.c_str(), "Content-Length:");
			if(!content_length) {
				content_length = strstr(conn->response.c_str(), "Content-length:");
			}

			if(!content_length) {
				content_length = strstr(conn->response.c_str(), "content-length:");
			}

			if(content_length) {
				content_length += strlen("content-length:");
				const int payload_len = strtol(content_length, NULL, 10);
				if(payload_len > 0) {
					conn->expected_len = (end_headers - conn->response.c_str()) + payload_len + header_term_len;
				}
			}
		}
	}

	if(conn->expected_len != -1 && conn->response.size() >= conn->expected_len) {
		ASSERT_LOG(conn->expected_len == conn->response.size(), "UNEXPECTED RESPONSE SIZE " << conn->expected_len << " VS " << conn->response << " " << conn->response.size());

		//TODO: handle message
		const char* end_headers = strstr(conn->response.c_str(), "\n\n");
		int header_term_len = 2;
		if(!end_headers) {
			header_term_len = 4;
			end_headers = strstr(conn->response.c_str(), "\r\n\r\n");
		}
		ASSERT_LOG(end_headers, "COULD NOT FIND END OF HEADERS IN MESSAGE: " << conn->response);
		conn->callable->add("message", variant(std::string(end_headers+header_term_len)));
		conn->handler("message_received");
	} else {
		conn->socket.async_read_some(boost::asio::buffer(conn->buf), boost::bind(&client::handle_receive, this, conn, _1, _2));
	}
}

void client::process()
{
	io_service_.poll();
	io_service_.reset();
}

variant client::get_value(const std::string& key) const
{
	return variant();
}

}
