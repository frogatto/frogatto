#include <GL/gl.h>

#include <map>
#include <sstream>
#include <stdio.h>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "filesystem.hpp"
#include "formatter.hpp"
#include "preferences.hpp"
#include "stats.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

namespace {
std::string get_stats_dir() {
	return sys::get_dir(std::string(preferences::user_data_path()) + "stats/") + "/";
}

}

void http_upload(const std::string& payload, const std::string& script) {
	using boost::asio::ip::tcp;

	std::ostringstream s;
	std::string header =
	    "POST /cgi-bin/" + script + " HTTP/1.1\n"
	    "Host: www.wesnoth.org\n"
	    "User-Agent: Frogatto 0.1\n"
	    "Content-Type: text/plain\n";
	s << header << "Content-length: " << payload.size() << "\n\n" << payload;
	std::string msg = s.str();

	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query("www.wesnoth.org", "80");

	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;

	tcp::socket socket(io_service);
	boost::system::error_code error = boost::asio::error::host_not_found;
	while(error && endpoint_iterator != end) {
		socket.close();
		socket.connect(*endpoint_iterator++, error);
	}

	if(error) {
		fprintf(stderr, "STATS ERROR: Can't resolve stats upload\n");
		return;
	}

	socket.write_some(boost::asio::buffer(msg), error);
	if(error) {
		fprintf(stderr, "STATS ERROR: Couldn't upload stats buffer\n");
		return;
	}
}

namespace stats {

namespace {
std::map<std::string, std::vector<const_record_ptr> > write_queue;

threading::mutex& write_queue_mutex() {
	static threading::mutex m;
	return m;
}

threading::condition& send_stats_signal() {
	static threading::condition c;
	return c;
}

bool send_stats_done = false;

void send_stats(const std::map<std::string, std::vector<const_record_ptr> >& queue) {
	if(queue.empty()) {
		return;
	}

	wml::node_ptr msg(new wml::node("stats"));
	for(std::map<std::string, std::vector<const_record_ptr> >::const_iterator i = queue.begin(); i != queue.end(); ++i) {
		std::string commands;
		wml::node_ptr cmd(new wml::node("level"));
		cmd->set_attr("id", i->first);
		for(std::vector<const_record_ptr>::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
			wml::node_ptr node((*j)->write());
			cmd->add_child(node);
			wml::write(node, commands);
		}

		msg->add_child(cmd);

		const std::string fname = get_stats_dir() + i->first;
		if(sys::file_exists(fname)) {
			commands = sys::read_file(fname) + commands;
		}

		sys::write_file(fname, commands);
	}

	std::string msg_str;
	wml::write(msg, msg_str);
	try {
		http_upload(msg_str, "upload-frogatto");
	} catch(...) {
		fprintf(stderr, "STATS ERROR: ERROR PERFORMING HTTP UPLOAD!\n");
	}
}

void send_stats_thread() {
	for(;;) {
		std::map<std::string, std::vector<const_record_ptr> > queue;
		{
			threading::lock lck(write_queue_mutex());
			if(!send_stats_done && write_queue.empty()) {
				send_stats_signal().wait_timeout(write_queue_mutex(), 60000);
			}

			if(send_stats_done && write_queue.empty()) {
				break;
			}

			queue.swap(write_queue);
		}

		send_stats(queue);
	}
}

}

bool download(const std::string& lvl) {
	try {
	using boost::asio::ip::tcp;

	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query("www.wesnoth.org", "80");

	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;

	tcp::socket socket(io_service);
	boost::system::error_code error = boost::asio::error::host_not_found;
	while(error && endpoint_iterator != end) {
		socket.close();
		socket.connect(*endpoint_iterator++, error);
	}

	if(error) {
		fprintf(stderr, "STATS ERROR: Can't resolve stats download\n");
		return false;
	}

	std::string query_str =
	    "GET /files/dave/frogatto-stats/" + lvl + " HTTP/1.1\n"
	    "Host: www.wesnoth.org\n"
	    "Connection: close\n\n";
	socket.write_some(boost::asio::buffer(query_str), error);
	if(error) {
		fprintf(stderr, "STATS ERROR: Error sending HTTP request\n");
		return false;
	}
	
	std::string payload;

	size_t nbytes;
	boost::array<char, 256> buf;
	while(!error && (nbytes = socket.read_some(boost::asio::buffer(buf), error)) > 0) {
		payload.insert(payload.end(), buf.begin(), buf.begin() + nbytes);
	}

	if(error != boost::asio::error::eof) {
		fprintf(stderr, "STATS ERROR: ERROR READING HTTP\n");
		return false;
	}

	fprintf(stderr, "REQUEST: {{{%s}}}\n\nRESPONSE: {{{%s}}}\n", query_str.c_str(), payload.c_str());

	const std::string expected_response = "HTTP/1.1 200 OK";
	if(payload.size() < expected_response.size() || std::equal(expected_response.begin(), expected_response.end(), payload.begin()) == false) {
		fprintf(stderr, "STATS ERROR: BAD HTTP RESPONSE\n");
		return false;
	}

	const std::string length_str = "Content-Length: ";
	const char* length_ptr = strstr(payload.c_str(), length_str.c_str());
	if(!length_ptr) {
		fprintf(stderr, "STATS ERROR: LENGTH NOT FOUND IN HTTP RESPONSE\n");
		return false;
	}

	length_ptr += length_str.size();

	const int len = atoi(length_ptr);
	if(len <= 0 || payload.size() <= len) {
		fprintf(stderr, "STATS ERROR: BAD LENGTH IN HTTP RESPONSE\n");
		return false;
	}

	std::string stats_wml = std::string(payload.end() - len, payload.end());

	sys::write_file(get_stats_dir() + lvl, stats_wml);
	return true;
	} catch(...) {
		fprintf(stderr, "STATS ERROR: ERROR PERFORMING STATS DOWNLOAD\n");
		return false;
	}
}

manager::manager() // : background_thread_(send_stats_thread)
{}

manager::~manager() {
	threading::lock lck(write_queue_mutex());
	send_stats_done = true;
	send_stats_signal().notify_one();
}

record_ptr record::read(wml::const_node_ptr node) {
	if(node->name() == "die") {
		return record_ptr(new die_record(point(node->attr("pos"))));
	} else if(node->name() == "quit") {
		return record_ptr(new quit_record(point(node->attr("pos"))));
	} else if(node->name() == "move") {
		return record_ptr(new player_move_record(point(node->attr("src")), point(node->attr("dst"))));
	} else if(node->name() == "load") {
		return record_ptr(new load_level_record(wml::get_int(node, "ms")));
	} else {
		fprintf(stderr, "UNRECOGNIZED STATS NODE: '%s'\n", node->name().c_str());
		return record_ptr();
	}
}

record::~record() {
}

die_record::die_record(const point& p) : p_(p)
{}

wml::node_ptr die_record::write() const
{
	wml::node_ptr result(new wml::node("die"));
	result->set_attr("pos", p_.to_string());
	return result;
}

void die_record::draw() const
{
	glPointSize(5);
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4ub(255, 0, 0, 255);
	GLfloat point[] = {p_.x, p_.y};
	glVertexPointer(2, GL_FLOAT, 0, point);
	glDrawArrays(GL_POINTS, 0, 1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glColor4ub(255, 255, 255, 255);
}

quit_record::quit_record(const point& p) : p_(p)
{}

wml::node_ptr quit_record::write() const
{
	wml::node_ptr result(new wml::node("quit"));
	result->set_attr("pos", p_.to_string());
	return result;
}

void quit_record::draw() const
{
	glPointSize(5);
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4ub(255, 255, 0, 255);
	GLfloat point[] = {p_.x, p_.y};
	glVertexPointer(2, GL_FLOAT, 0, point);
	glDrawArrays(GL_POINTS, 0, 1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glColor4ub(255, 255, 255, 255);
}

player_move_record::player_move_record(const point& src, const point& dst) : src_(src), dst_(dst)
{}

wml::node_ptr player_move_record::write() const
{
	wml::node_ptr result(new wml::node("move"));
	result->set_attr("src", src_.to_string());
	result->set_attr("dst", dst_.to_string());
	return result;
}

void player_move_record::draw() const
{
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4ub(0, 0, 255, 128);
	GLfloat varray[] = {src_.x, src_.y, dst_.x, dst_.y};
	glVertexPointer(2, GL_FLOAT, 0, varray);
	glDrawArrays(GL_LINES, 0, 4);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glColor4ub(255, 255, 255, 255);
}

load_level_record::load_level_record(int ms) : ms_(ms)
{}

wml::node_ptr load_level_record::write() const
{
	wml::node_ptr result(new wml::node("load"));
	result->set_attr("ms", formatter() << ms_);
	return result;
}

void record_event(const std::string& lvl, const_record_ptr r)
{
	threading::lock lck(write_queue_mutex());
	write_queue[lvl].push_back(r);
}

void flush()
{
	send_stats_signal().notify_one();
}

}
