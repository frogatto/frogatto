#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <iostream>

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "json_parser.hpp"
#include "string_utils.hpp"
#include "tbs_server.hpp"
#include "tbs_web_server.hpp"
#include "unit_test.hpp"
#include "variant.hpp"

namespace tbs {

#ifdef _WINDOWS
namespace {
const __int64 DELTA_EPOCH_IN_MICROSECS= 11644473600000000;

struct timezone2 
{
  __int32  tz_minuteswest; /* minutes W of Greenwich */
  bool  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone2 *tz)
{
	if(tv) {
		FILETIME ft;
		__int64 tmpres = 0;
		ZeroMemory(&ft,sizeof(ft));
		GetSystemTimeAsFileTime(&ft);

		tmpres = ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres /= 10;  /*convert into microseconds*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS; 
		tv->tv_sec = (__int32)(tmpres * 0.000001);
		tv->tv_usec =(tmpres % 1000000);
	}

    //_tzset(),don't work properly, so we use GetTimeZoneInformation
	if(tz) {
		TIME_ZONE_INFORMATION tz_winapi;
		ZeroMemory(&tz_winapi, sizeof(tz_winapi));
		int rez = GetTimeZoneInformation(&tz_winapi);
		tz->tz_dsttime = (rez == 2) ? true : false;
		tz->tz_minuteswest = tz_winapi.Bias + ((rez == 2) ? tz_winapi.DaylightBias : 0);
	}
	return 0;
}

}
#endif 

std::string global_debug_str;

using boost::asio::ip::tcp;

web_server::web_server(boost::asio::io_service& io_service, server& serv, int port)
  : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
    server_(serv)
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
		variant doc;

		try {
			doc = json::parse(std::string(payload), json::JSON_NO_PREPROCESSOR);
		} catch(...) {
		}

		if(!doc.is_null()) {
			int session_id = -1;
			std::map<std::string, std::string>::const_iterator i = env.find("cookie");
			if(i != env.end()) {
				const char* cookie_start = strstr(i->second.c_str(), " session=");
				if(cookie_start != NULL) {
					++cookie_start;
				} else {
					cookie_start = strstr(i->second.c_str(), "session=");
					if(cookie_start != i->second.c_str()) {
						cookie_start = NULL;
					}
				}

				if(cookie_start) {
					session_id = atoi(cookie_start+8);
				}
			}

			if(doc["debug_session"].is_bool()) {
				session_id = doc["debug_session"].as_bool();
			}

			server_.adopt_ajax_socket(socket, session_id, doc);
			return;
		}
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
	std::stringstream buf;
	buf << 
		"HTTP/1.1 200 OK\r\n"
		"Date: Tue, 20 Sep 2011 21:00:00 GMT\r\n"
		"Connection: close\r\n"
		"Server: Wizard/1.0\r\n"
		"Accept-Ranges: none\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Content-Type: " << type << "\r\n"
		"Last-Modified: Tue, 20 Sep 2011 10:00:00 GMT\r\n"
		<< header_parms << "\r\n";

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
		"Date: Tue, 20 Sep 2011 21:00:00 GMT\r\n"
		"Connection: close\r\n"
		"Server: Wizard/1.0\r\n"
		"Accept-Ranges: none\r\n"
		"\r\n";
	boost::shared_ptr<std::string> str(new std::string(buf.str()));
	boost::asio::async_write(*socket, boost::asio::buffer(*str),
                boost::bind(&web_server::handle_send, this, socket, _1, _2, str->size(), str));

}

}

COMMAND_LINE_UTILITY(tbs_server) {
	int port = 23456;
	if(args.size() > 0) {
		std::vector<std::string>::const_iterator it = args.begin();
		while(it != args.end()) {
			if(*it == "--port" || *it == "--listen-port") {
				it++;
				if(it != args.end()) {
					port = atoi(it->c_str());
					ASSERT_LOG(port > 0 && port <= 65535, "tbs_server(): Port must lie in the range 1-65535.");
					it = args.end();
				}
			} else {
				it++;
			}
		}
	}
	std::cerr << "tbs_server(): Listening on port " << std::dec << port << std::endl;
	boost::asio::io_service io_service;
	tbs::server s(io_service);
	tbs::web_server ws(io_service, s, port);
	io_service.run();
}
