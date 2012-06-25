#include <boost/bind.hpp>

#include "asserts.hpp"
#include "tbs_client.hpp"

namespace tbs {

client::client(const std::string& host, const std::string& port, int session)
  : session_id_(session),
    resolver_(io_service_),
    resolver_query_(host.c_str(), port.c_str()),
	endpoint_iterator_(resolver_.resolve(resolver_query_))
{
}

void client::send_request(const std::string& request, boost::function<void(std::string)> handler)
{
	connection_ptr conn(new Connection(io_service_));
	conn->request = request;
	conn->handler = handler;
	conn->socket.async_connect(*endpoint_iterator_,
        boost::bind(&client::handle_connect, this,
          boost::asio::placeholders::error, conn, endpoint_iterator_));

}

void client::handle_connect(const boost::system::error_code& error, connection_ptr conn, tcp::resolver::iterator resolve_itor)
{
	if(error) {
		if(endpoint_iterator_ == resolve_itor) {
			++endpoint_iterator_;
		}
		ASSERT_LOG(endpoint_iterator_ != tcp::resolver::iterator(), "COULD NOT RESOLVE TBS SERVER: " << endpoint_iterator_->endpoint().address().to_string() << ":" << endpoint_iterator_->endpoint().port());

		conn->socket.async_connect(*endpoint_iterator_,
 	       boost::bind(&client::handle_connect, this,
	          boost::asio::placeholders::error, conn, endpoint_iterator_));
		return;
	}

	//do async write.
	std::ostringstream msg;
	msg << "POST /tbs HTTP/1.1\n"
	       "User-Agent: Frogatto 1.0\n"
		   "Content-Type; text/plain\n";
	
	if(session_id_ != -1) {
		msg << "Cookie: session=" << session_id_ << "\n";
	}

	msg << "Content-length: " << conn->request.size() << "\n\n" <<
		   conn->request;
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
	ASSERT_LOG(!e, "ERROR RECEIVING DATA");
	conn->response.insert(conn->response.end(), &conn->buf[0], &conn->buf[0] + nbytes);
	if(conn->expected_len == -1) {
		const char* end_headers = strstr(conn->response.c_str(), "\n\n");
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
					conn->expected_len = (end_headers - conn->response.c_str()) + payload_len + 2;
				}
			}
		}
	}

	if(conn->expected_len != -1 && conn->response.size() >= conn->expected_len) {
		ASSERT_LOG(conn->expected_len == conn->response.size(), "UNEXPECTED RESPONSE SIZE " << conn->expected_len << " VS " << conn->response << " " << conn->response.size());

		//TODO: handle message
		const char* end_headers = strstr(conn->response.c_str(), "\n\n");
		ASSERT_LOG(end_headers, "COULD NOT FIND END OF HEADERS IN MESSAGE: " << conn->response);
		conn->handler(std::string(end_headers+2));
	} else {
		conn->socket.async_read_some(boost::asio::buffer(conn->buf), boost::bind(&client::handle_receive, this, conn, _1, _2));
	}
}

void client::process()
{
	io_service_.poll();
}

variant client::get_value(const std::string& key) const
{
	return variant();
}

}
