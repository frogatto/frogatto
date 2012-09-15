#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "asserts.hpp"
#include "http_client.hpp"

http_client::http_client(const std::string& host, const std::string& port, int session)
  : session_id_(session),
    resolver_(io_service_),
	host_(host),
	resolver_query_(host.c_str(), port.c_str()),
	in_flight_(0)
{
}

void http_client::send_request(const std::string& method_path, const std::string& request, boost::function<void(std::string)> handler, boost::function<void(std::string)> error_handler, boost::function<void(int,int,bool)> progress_handler)
{
	++in_flight_;
	connection_ptr conn(new Connection(io_service_));
	conn->method_path = method_path;
	conn->request = request;
	conn->handler = handler;
	conn->error_handler = error_handler;
	conn->progress_handler = progress_handler;

	resolver_.async_resolve(resolver_query_,
		boost::bind(&http_client::handle_resolve, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::iterator,
			conn));
}

void http_client::handle_resolve(const boost::system::error_code& error, tcp::resolver::iterator endpoint_iterator, connection_ptr conn)
{
	if(!error)
	{
		endpoint_iterator_ = endpoint_iterator;
		// Attempt a connection to each endpoint in the list until we
		// successfully establish a connection.
#if BOOST_VERSION >= 104700
		boost::asio::async_connect(conn->socket, 
			endpoint_iterator_,
			boost::bind(&http_client::handle_connect, this,
				boost::asio::placeholders::error, conn, endpoint_iterator_));
#else
		conn->socket.async_connect(*endpoint_iterator_,
			boost::bind(&http_client::handle_connect, this,
				boost::asio::placeholders::error, conn, endpoint_iterator_));
#endif
	} else {
		--in_flight_;
		conn->error_handler("Error resolving connection");
	}
}

void http_client::handle_connect(const boost::system::error_code& error, connection_ptr conn, tcp::resolver::iterator resolve_itor)
{
	if(error) {
		std::cerr << "HANDLE_CONNECT_ERROR: " << error << std::endl;
		if(endpoint_iterator_ == resolve_itor) {
			++endpoint_iterator_;
		}
		//ASSERT_LOG(endpoint_iterator_ != tcp::resolver::iterator(), "COULD NOT RESOLVE TBS SERVER: " << resolve_itor->endpoint().address().to_string() << ":" << resolve_itor->endpoint().port());
		if(endpoint_iterator_ == tcp::resolver::iterator()) {
			--in_flight_;
			conn->error_handler("Error establishing connection");
			return;
		}

#if BOOST_VERSION >= 104700
		boost::asio::async_connect(conn->socket, 
			endpoint_iterator_,
			boost::bind(&http_client::handle_connect, this,
				boost::asio::placeholders::error, conn, endpoint_iterator_));
#else
		conn->socket.async_connect(*endpoint_iterator_,
 	       boost::bind(&http_client::handle_connect, this,
	          boost::asio::placeholders::error, conn, endpoint_iterator_));
#endif
		return;
	}

	//do async write.
	std::ostringstream msg;
	msg << conn->method_path << " HTTP/1.1\r\n"
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

	conn->request = msg.str();

	write_connection_data(conn);
}

void http_client::write_connection_data(connection_ptr conn)
{
	const int bytes_to_send = conn->request.size() - conn->nbytes_sent;
	const int nbytes = std::min<int>(bytes_to_send, 1024*64);

	const std::string msg(conn->request.begin() + conn->nbytes_sent, conn->request.begin() + conn->nbytes_sent + nbytes);
	boost::asio::async_write(conn->socket, boost::asio::buffer(msg),
	      boost::bind(&http_client::handle_send, this, conn, _1, _2));

}

void http_client::handle_send(connection_ptr conn, const boost::system::error_code& e, size_t nbytes)
{

	if(e) {
		--in_flight_;
		if(conn->error_handler) {
			conn->error_handler("ERROR SENDING DATA");
		}

		return;
	}

	conn->nbytes_sent += nbytes;

	if(conn->progress_handler) {
		conn->progress_handler(conn->nbytes_sent, conn->request.size(), false);
	}

	if(conn->nbytes_sent < conn->request.size()) {
		write_connection_data(conn);
	} else {
		conn->socket.async_read_some(boost::asio::buffer(conn->buf), boost::bind(&http_client::handle_receive, this, conn, _1, _2));
	}
}

void http_client::handle_receive(connection_ptr conn, const boost::system::error_code& e, size_t nbytes)
{
	if(e) {
		--in_flight_;
		fprintf(stderr, "ERROR WITH (%d, %s)\n", e.value(), conn->response.c_str());
		if(e.value() == 2) {
			const char* end_headers = strstr(conn->response.c_str(), "\n\n");
			const char* end_headers2 = strstr(conn->response.c_str(), "\n\r\n\n");
			if(end_headers2 && (end_headers == NULL || end_headers2 < end_headers)) {
				end_headers = end_headers2 + 2;
			}

			if(end_headers) {
				conn->handler(std::string(end_headers+2));
				return;
			}
		}

		if(conn->error_handler) {
			conn->error_handler("ERROR RECEIVING DATA");
		}

		return;
	}

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

		//We have the full response now -- handle it.
		const char* end_headers = strstr(conn->response.c_str(), "\n\n");
		int header_term_len = 2;
		if(!end_headers) {
			header_term_len = 4;
			end_headers = strstr(conn->response.c_str(), "\r\n\r\n");
		}
		ASSERT_LOG(end_headers, "COULD NOT FIND END OF HEADERS IN MESSAGE: " << conn->response);
		--in_flight_;
		conn->handler(std::string(end_headers+header_term_len));
	} else {
		if(conn->expected_len != -1 && conn->progress_handler) {
			conn->progress_handler(conn->response.size(), conn->expected_len, true);
		}
		conn->socket.async_read_some(boost::asio::buffer(conn->buf), boost::bind(&http_client::handle_receive, this, conn, _1, _2));
	}
}

void http_client::process()
{
	io_service_.poll();
	io_service_.reset();
}

variant http_client::get_value(const std::string& key) const
{
	if(key == "in_flight") {
		return variant(in_flight_);
	}
	return variant();
}
