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
#include "module_web_server.hpp"
#include "string_utils.hpp"
#include "utils.hpp"
#include "unit_test.hpp"
#include "variant.hpp"

using boost::asio::ip::tcp;

module_web_server::module_web_server(const std::string& data_path, boost::asio::io_service& io_service, int port)
  : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
	timer_(io_service), nheartbeat_(0),
	data_path_(data_path)
{
	if(data_path_.empty() || data_path_[data_path_.size()-1] != '/') {
		data_path_ += "/";
	}

	start_accept();
	heartbeat();
}

module_web_server::~module_web_server()
{}

void module_web_server::start_accept()
{
	socket_ptr socket(new tcp::socket(acceptor_.get_io_service()));
	acceptor_.async_accept(*socket, boost::bind(&module_web_server::handle_accept, this, socket, boost::asio::placeholders::error));

}

namespace {
int nconnections = 0;
}

void module_web_server::handle_accept(socket_ptr socket, const boost::system::error_code& error)
{
	if(error) {
		std::cerr << "ERROR IN ACCEPT\n";
		return;
	}

	std::cerr << "RECEIVED CONNECTION. Listening " << ++nconnections << "\n";

	start_receive(socket);
	start_accept();
}

void module_web_server::start_receive(socket_ptr socket, receive_buf_ptr recv_buf)
{
	if(!recv_buf) {
		recv_buf.reset(new receive_buf);
	}

	buffer_ptr buf(new boost::array<char, 64*1024>);
	socket->async_read_some(boost::asio::buffer(*buf), boost::bind(&module_web_server::handle_receive, this, socket, buf, _1, _2, recv_buf));
}

void module_web_server::handle_receive(socket_ptr socket, buffer_ptr buf, const boost::system::error_code& e, size_t nbytes, receive_buf_ptr recv_buf)
{
	if(e) {
		//TODO: handle error
		std::cerr << "SOCKET ERROR: " << e.message() << "\n";
		std::cerr << "CLOSESOCKA\n";
		disconnect(socket);
		return;
	}

	handle_incoming_data(socket, &(*buf)[0], &(*buf)[0] + nbytes, recv_buf);

//	start_receive(socket);
}

void module_web_server::handle_incoming_data(socket_ptr socket, const char* i1, const char* i2, receive_buf_ptr recv_buf)
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

				std::cerr << "ARG: " << name << " -> " << value << "\n";
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

		std::cerr << "PARM: " << key << " -> " << value << "\n";
	}

	return env;
}

}

bool phpbb_check_username_password(std::string username, std::string password);

void module_web_server::handle_message(socket_ptr socket, receive_buf_ptr recv_buf)
{
	const std::string& msg = recv_buf->msg;
	if(msg.size() < 16) {
		std::cerr << "CLOSESOCKB\n";
		disconnect(socket);
		return;
	}

	if(std::equal(msg.begin(), msg.begin()+5, "POST ")) {

		variant doc;
		std::map<std::string, std::string> env = parse_env(msg);
		std::cerr << "CONTENT LENGTH: (" << env["content-length"] << ")\n";
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

		try {
			doc = json::parse(payload, json::JSON_NO_PREPROCESSOR);
		} catch(json::parse_error&) {
			std::cerr << "ERROR PARSING JSON\n";
		}

		if(!doc.is_null()) {
			handle_post(socket, doc);
			return;
		}
	} else if(std::equal(msg.begin(), msg.begin()+4, "GET ")) {
		std::string::const_iterator begin_url = msg.begin() + 4;
		std::string::const_iterator end_url = std::find(begin_url, msg.end(), ' ');
		std::string::const_iterator begin_args = std::find(begin_url, end_url, '?');
		std::map<std::string, std::string> args;
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

		handle_get(socket, std::string(begin_url, begin_args), args);

		return;
	}

	disconnect(socket);
}

void module_web_server::handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, size_t max_bytes, boost::shared_ptr<std::string> buf)
{
	std::cerr << "SENT: " << nbytes << " / " << max_bytes << " " << e << "\n";
		std::cerr << "CLOSESOCKF\n";
	if(nbytes == max_bytes || e) {
		disconnect(socket);
	}
}

void module_web_server::disconnect(socket_ptr socket)
{
	socket->close();
	--nconnections;
}

void module_web_server::send_msg(socket_ptr socket, const std::string& type, const std::string& msg, const std::string& header_parms)
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
	                         boost::bind(&module_web_server::handle_send, this, socket, _1, _2, str->size(), str));
}

void module_web_server::send_404(socket_ptr socket)
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
                boost::bind(&module_web_server::handle_send, this, socket, _1, _2, str->size(), str));

}

void module_web_server::heartbeat()
{
	timer_.expires_from_now(boost::posix_time::seconds(1));
	timer_.async_wait(boost::bind(&module_web_server::heartbeat, this));
}

void module_web_server::handle_post(socket_ptr socket, variant doc)
{
	std::map<variant,variant> response;
	try {
		const std::string msg_type = doc["type"].as_string();
		if(msg_type == "upload_module") {
			variant module_node = doc["module"];
			const std::string module_id = module_node["id"].as_string();
			ASSERT_LOG(std::count_if(module_id.begin(), module_id.end(), isalnum) + std::count(module_id.begin(), module_id.end(), '_') == module_id.size(), "ILLEGAL MODULE ID");

			const std::string module_path = data_path_ + module_id + ".cfg";
			const std::string module_path_tmp = module_path + ".tmp";
			const std::string contents = module_node.write_json();
			
			sys::write_file(module_path_tmp, contents);
			const int rename_result = rename(module_path_tmp.c_str(), module_path.c_str());
			ASSERT_LOG(rename_result == 0, "FAILED TO RENAME FILE: " << errno);

			response[variant("status")] = variant("ok");

		} else {
			ASSERT_LOG(false, "Unknown message type");
		}
	} catch(validation_failure_exception& e) {
		response[variant("status")] = variant("error");
		response[variant("message")] = variant(e.msg);
	}

	send_msg(socket, "text/json", variant(&response).write_json(), "");
}

void module_web_server::handle_get(socket_ptr socket, const std::string& url, const std::map<std::string, std::string>& args)
{
	std::map<variant,variant> response;
	send_msg(socket, "text/json", variant(&response).write_json(), "");
}

COMMAND_LINE_UTILITY(module_server)
{
	std::string path = ".";
	int port = 23456;

	std::deque<std::string> arguments(args.begin(), args.end());
	while(!arguments.empty()) {
		const std::string arg = arguments.front();
		arguments.pop_front();
		if(arg == "--path") {
			ASSERT_LOG(arguments.empty() == false, "NEED ARGUMENT AFTER " << arg);
			path = arguments.front();
			arguments.pop_front();
		} else if(arg == "-p" || arg == "--port") {
			ASSERT_LOG(arguments.empty() == false, "NEED ARGUMENT AFTER " << arg);
			port = atoi(arguments.front().c_str());
			arguments.pop_front();
		} else {
			ASSERT_LOG(false, "UNRECOGNIZED ARGUMENT: " << arg);
		}
	}

	const assert_recover_scope recovery;
	boost::asio::io_service io_service;
	module_web_server server(path, io_service, port);
	io_service.run();
}
