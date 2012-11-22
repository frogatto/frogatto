#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <deque>
#include <iostream>

#if !defined(_WINDOWS)
#include <sys/time.h>
#endif

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "json_parser.hpp"
#include "http_server.hpp"
#include "string_utils.hpp"
#include "utils.hpp"
#include "unit_test.hpp"
#include "variant.hpp"

using boost::asio::ip::tcp;

namespace http {

web_server::web_server(boost::asio::io_service& io_service, int port)
  : acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
{
	start_accept();
}

void web_server::start_accept()
{
	socket_ptr socket(new tcp::socket(acceptor_.get_io_service()));
	acceptor_.async_accept(*socket, boost::bind(&web_server::handle_accept, this, socket, boost::asio::placeholders::error));

}

namespace {
int nconnections = 0;
}

void web_server::handle_accept(socket_ptr socket, const boost::system::error_code& error)
{
	if(error) {
		std::cerr << "ERROR IN ACCEPT\n";
		return;
	}

	start_receive(socket);
	start_accept();
}

void web_server::start_receive(socket_ptr socket, receive_buf_ptr recv_buf)
{
	if(!recv_buf) {
		recv_buf.reset(new receive_buf);
	}

	buffer_ptr buf(new boost::array<char, 64*1024>);
	socket->async_read_some(boost::asio::buffer(*buf), boost::bind(&web_server::handle_receive, this, socket, buf, _1, _2, recv_buf));
}

void web_server::handle_receive(socket_ptr socket, buffer_ptr buf, 
	const boost::system::error_code& e, 
	size_t nbytes, 
	receive_buf_ptr recv_buf)
{
	if(e) {
		//TODO: handle error
		std::cerr << "SOCKET ERROR: " << e.message() << "\n";
		disconnect(socket);
		return;
	}

	handle_incoming_data(socket, &(*buf)[0], &(*buf)[0] + nbytes, recv_buf);
}

void web_server::handle_incoming_data(socket_ptr socket, const char* i1, const char* i2, receive_buf_ptr recv_buf)
{
	recv_buf->msg += std::string(i1, i2);

	if(recv_buf->wanted > 0 && recv_buf->msg.size() < recv_buf->wanted) {
		start_receive(socket, recv_buf);
		return;
	}

	timeval before, after;
	gettimeofday(&before, NULL);
	handle_message(socket, recv_buf);
	gettimeofday(&after, NULL);

	const int ms = (after.tv_sec - before.tv_sec)*1000 + (after.tv_usec - before.tv_usec)/1000;
	std::cerr << "handle_incoming_data time: " << ms << "ms\n";
}

namespace {
struct Request {
	std::string path;
	std::map<std::string, std::string> args;
};

Request parse_request(const std::string& str) {
	std::string::const_iterator end_path = std::find(str.begin(), str.end(), '?');
	Request request;
	request.path.assign(str.begin(), end_path);

	std::cerr << "PATH: '" << request.path << "'\n";

	if(end_path != str.end()) {
		++end_path;

		std::vector<std::string> args = util::split(std::string(end_path, str.end()), '&');
		foreach(const std::string& a, args) {
			std::string::const_iterator equal_itor = std::find(a.begin(), a.end(), '=');
			if(equal_itor != a.end()) {
				const std::string name(a.begin(), equal_itor);
				const std::string value(equal_itor+1, a.end());
				request.args[name] = value;
			}
		}
	}

	return request;
}

std::map<std::string, std::string> parse_env(const std::string& str)
{
	std::map<std::string, std::string> env;
	std::vector<std::string> lines = util::split(str, '\n');
	foreach(const std::string& line, lines) {
		if(line.empty()) {
			break;
		}

		std::string::const_iterator colon = std::find(line.begin(), line.end(), ':');
		if(colon == line.end() || colon+1 == line.end()) {
			continue;
		}

		std::string key(line.begin(), colon);
		const std::string value(colon+2, line.end());

		std::transform(key.begin(), key.end(), key.begin(), tolower);
		env[key] = value;
	}

	return env;
}

}

void web_server::handle_message(socket_ptr socket, receive_buf_ptr recv_buf)
{
	const std::string& msg = recv_buf->msg;
	if(msg.size() < 16) {
		std::cerr << "CLOSESOCKB\n";
		disconnect(socket);
		return;
	}

	if(std::equal(msg.begin(), msg.begin()+5, "POST ")) {

		environment env = parse_env(msg);
		const int content_length = atoi(env["content-length"].c_str());

		const char* payload = NULL;
		const char* payload1 = strstr(msg.c_str(), "\n\n");
		const char* payload2 = strstr(msg.c_str(), "\r\n\r\n");
		if(payload1) {
			payload1 += 2;
			payload = payload1;
		}

		if(payload2 && (!payload1 || payload2 < payload1)) {
			payload2 += 4;
			payload = payload2;
		}

		const int payload_len = payload ? (msg.c_str() + msg.size() - payload) : 0;

		if(!payload || payload_len < content_length) {
			if(payload_len) {
				recv_buf->wanted = msg.size() + (content_length - payload_len);
			}
			start_receive(socket, recv_buf);
			return;
		}

		variant doc;

		try {
			doc = json::parse(std::string(payload), json::JSON_NO_PREPROCESSOR);
		} catch(...) {
			std::cerr << "ERROR PARSING JSON\n";
		}

		if(!doc.is_null()) {
			handle_post(socket, doc, env);
			return;
		}
	} else if(std::equal(msg.begin(), msg.begin()+4, "GET ")) {
		std::string::const_iterator begin_url = msg.begin() + 4;
		std::string::const_iterator end_url = std::find(begin_url, msg.end(), ' ');
		std::string::const_iterator begin_args = std::find(begin_url, end_url, '?');
		std::map<std::string, std::string> args;
		std::string url_base(begin_url, begin_args);
		if(begin_args != end_url) {
			begin_args++;
			while(begin_args != end_url) {
				std::string::const_iterator eq = std::find(begin_args, end_url, '=');
				if(eq == end_url) {
					break;
				}

				std::string::const_iterator amp = std::find(eq, end_url, '&');
				std::string name(begin_args, eq);
				std::string value(eq+1, amp);
				args[name] = value;

				begin_args = amp;
				if(begin_args == end_url) {
					break;
				}

				++begin_args;
			}
		}

		handle_get(socket, url_base, args);

		return;
	}

	disconnect(socket);
}

void web_server::handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, size_t max_bytes, boost::shared_ptr<std::string> buf)
{
	if(nbytes == max_bytes || e) {
		disconnect(socket);
	}
}

void web_server::disconnect_socket(socket_ptr socket)
{
	socket->close();
	--nconnections;
}

void web_server::disconnect(socket_ptr socket)
{
	disconnect_socket(socket);
}

void web_server::send_msg(socket_ptr socket, const std::string& type, const std::string& msg, const std::string& header_parms)
{
    std::stringstream buf;
	buf <<
		"HTTP/1.1 200 OK\r\n"
		"Date: " << get_http_datetime() << "\r\n"
		"Connection: close\r\n"
		"Server: Wizard/1.0\r\n"
		"Accept-Ranges: bytes\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Content-Type: " << type << "\r\n"
		"Content-Length: " << std::dec << (int)msg.size() << "\r\n"
		"Last-Modified: " << get_http_datetime() << "\r\n" <<
        (header_parms.empty() ? "" : header_parms + "\r\n")
        << "\r\n";

	boost::shared_ptr<std::string> str(new std::string(buf.str()));
	*str += msg;

	boost::asio::async_write(*socket, boost::asio::buffer(*str),
	                         boost::bind(&web_server::handle_send, this, socket, _1, _2, str->size(), str));
}

void web_server::send_404(socket_ptr socket)
{
	std::stringstream buf;
	buf << 
		"HTTP/1.1 404 NOT FOUND\r\n"
		"Date: " << get_http_datetime() << "\r\n"
		"Connection: close\r\n"
		"Server: Wizard/1.0\r\n"
		"Accept-Ranges: none\r\n"
		"\r\n";
	boost::shared_ptr<std::string> str(new std::string(buf.str()));
	boost::asio::async_write(*socket, boost::asio::buffer(*str),
                boost::bind(&web_server::handle_send, this, socket, _1, _2, str->size(), str));
}

}
