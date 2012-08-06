#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <iostream>

#include <sys/time.h>

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "json_parser.hpp"
#include "stats_server.hpp"
#include "stats_web_server.hpp"
#include "string_utils.hpp"
#include "variant.hpp"

std::string global_debug_str;

using boost::asio::ip::tcp;

web_server::web_server(boost::asio::io_service& io_service, int port)
  : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
	timer_(io_service), nheartbeat_(0)
{
	start_accept();
	heartbeat();
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

	std::cerr << "RECEIVED CONNECTION. Listening " << ++nconnections << "\n";

	start_receive(socket);

	start_accept();
}

void web_server::start_receive(socket_ptr socket, receive_buf_ptr recv_buf)
{
	if(!recv_buf) {
		recv_buf.reset(new receive_buf);
	}

	buffer_ptr buf(new boost::array<char, 1024>);
	socket->async_read_some(boost::asio::buffer(*buf), boost::bind(&web_server::handle_receive, this, socket, buf, _1, _2, recv_buf));
}

void web_server::handle_receive(socket_ptr socket, buffer_ptr buf, const boost::system::error_code& e, size_t nbytes, receive_buf_ptr recv_buf)
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

void web_server::handle_message(socket_ptr socket, receive_buf_ptr recv_buf)
{
	const std::string& msg = recv_buf->msg;
	if(msg.size() < 16) {
		std::cerr << "CLOSESOCKB\n";
		disconnect(socket);
		return;
	}

	std::cerr << "MESSAGE RECEIVED: " << msg << "\n";

	if(std::equal(msg.begin(), msg.begin()+5, "POST ")) {

		variant doc;
		std::map<std::string, std::string> env = parse_env(msg);
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

		std::cerr << "PAYLOAD: " << (payload ? payload_len : 0) << " VS " << content_length << "\n";
		
		if(!payload || payload_len < content_length) {
			if(payload_len) {
				recv_buf->wanted = msg.size() + (content_length - payload_len);
			}
			start_receive(socket, recv_buf);
			return;
		}

		std::cerr << "POST: (((" << payload << ")))\n";

		try {
			doc = json::parse(payload, json::JSON_NO_PREPROCESSOR);
		} catch(json::parse_error&) {
			std::cerr << "ERROR PARSING JSON\n";
		}

		if(!doc.is_null()) {

			static const variant TypeVariant("type");
			const std::string& type = doc[TypeVariant].as_string();
			if(type == "stats") {
				process_stats(doc);
			} else if(type == "upload_table_definitions") {
				//TODO: add authentication to get info about the user
				//and make sure they have permission to update this module.
				const std::string& module = doc[variant("module")].as_string();
				init_tables_for_module(module, doc[variant("definition")]);

				try {
					send_msg(socket, "text/json", "{ \"status\": \"ok\" }", "");
				} catch(validation_failure_exception& e) {
					std::map<variant,variant> msg;
					msg[variant("status")] = variant("error");
					msg[variant("message")] = variant(e.msg);
					send_msg(socket, "text/json", variant(&msg).write_json(), "");
				}

				return;
			}
			disconnect(socket);

		}
	} else if(std::equal(msg.begin(), msg.begin()+4, "GET ")) {
		std::cerr << "MESSAGE: (((" << msg << ")))\n";
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

				std::cerr << "ARG: (" << name << ") -> (" << value << ")\n";

				begin_args = amp;
				if(begin_args == end_url) {
					break;
				}

				++begin_args;
			}
		}

		variant value = get_stats(args["version"], args["module"], args["module_version"], args["level"]);
		send_msg(socket, "text/plain", value.write_json(true), "");
		return;
	}

	disconnect(socket);
}

void web_server::handle_send(socket_ptr socket, const boost::system::error_code& e, size_t nbytes, size_t max_bytes, boost::shared_ptr<std::string> buf)
{
	std::cerr << "SENT: " << nbytes << " / " << max_bytes << " " << e << "\n";
		std::cerr << "CLOSESOCKF\n";
	if(nbytes == max_bytes || e) {
		disconnect(socket);
	}
}

void web_server::disconnect(socket_ptr socket)
{
	socket->close();
	--nconnections;
}

void web_server::send_msg(socket_ptr socket, const std::string& type, const std::string& msg, const std::string& header_parms)
{
	char buf[4096*4];
	sprintf(buf, "HTTP/1.1 200 OK\nDate: Tue, 20 Sep 2011 21:00:00 GMT\nConnection: close\nServer: Wizard/1.0\nAccept-Ranges: none\nAccess-Control-Allow-Origin: *\nContent-Type: %s\nContent-Length: %d\nLast-Modified: Tue, 20 Sep 2011 10:00:00 GMT%s\n\n", type.c_str(), msg.size(), (header_parms.empty() ? "" : ("\n" + header_parms).c_str()));


	boost::shared_ptr<std::string> str(new std::string(buf));
	*str += msg;

	boost::asio::async_write(*socket, boost::asio::buffer(*str),
	                         boost::bind(&web_server::handle_send, this, socket, _1, _2, str->size(), str));
}

void web_server::send_404(socket_ptr socket)
{
	char buf[4096];
	sprintf(buf, "HTTP/1.1 404 NOT FOUND\nDate: Tue, 20 Sep 2011 21:00:00 GMT\nConnection: close\nServer: Wizard/1.0\nAccept-Ranges: none\n\n");
	boost::shared_ptr<std::string> str(new std::string(buf));
	boost::asio::async_write(*socket, boost::asio::buffer(*str),
                boost::bind(&web_server::handle_send, this, socket, _1, _2, str->size(), str));

}

void web_server::heartbeat()
{
	if(++nheartbeat_%3600 == 0) {
		std::cerr << "WRITING DATA...\n";
		timeval start_time, end_time;
		gettimeofday(&start_time, NULL);
		variant v = write_stats();
		std::string data = v.write_json(true);

		if(sys::file_exists("stats-5.json")) {
			sys::remove_file("stats-5.json");
		}

		for(int n = 4; n >= 1; --n) {
			if(sys::file_exists(formatter() << "stats-" << n << ".json")) {
				sys::move_file(formatter() << "stats-" << n << ".json",
				               formatter() << "stats-" << (n+1) << ".json");
			}
		}

		sys::write_file("stats-1.json", data);

		gettimeofday(&end_time, NULL);

		const int time_us = (end_time.tv_sec - start_time.tv_sec)*1000000 + (end_time.tv_usec - start_time.tv_usec);
		std::cerr << "WROTE STATS IN " << time_us << "us\n";
	}

	timer_.expires_from_now(boost::posix_time::seconds(1));
	timer_.async_wait(boost::bind(&web_server::heartbeat, this));
}
