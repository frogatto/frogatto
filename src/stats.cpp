#include "graphics.hpp"
#include <map>
#include <sstream>
#include <stdio.h>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "filesystem.hpp"
#include "formatter.hpp"
#include "level.hpp"
#include "preferences.hpp"
#include "playable_custom_object.hpp"
#include "stats.hpp"

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
	tcp::resolver::query query("theargentlark.com", "5000");

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

	//std::cerr << "STATS UPLOADED TO " << script << ": \n" << payload << "\n";
}

namespace stats {

namespace {
std::map<std::string, std::vector<variant> > write_queue;

std::vector<std::pair<std::string, std::string> > upload_queue;

threading::mutex& upload_queue_mutex() {
	static threading::mutex m;
	return m;
}

threading::condition& send_stats_signal() {
	static threading::condition c;
	return c;
}

bool send_stats_should_exit = false;

void send_stats(std::map<std::string, std::vector<variant> >& queue) {
	if(queue.empty()) {
		return;
	}

	std::map<variant, variant> attr;
	attr[variant("type")] = variant("stats");
	attr[variant("version")] = variant(preferences::version());
	attr[variant("user_id")] = variant(preferences::get_unique_user_id());

	std::vector<variant> level_vec;

	for(std::map<std::string, std::vector<variant> >::iterator i = queue.begin(); i != queue.end(); ++i) {

		std::map<variant, variant> obj;
		obj[variant("level")] = variant(i->first);
		obj[variant("stats")] = variant(&i->second);
		level_vec.push_back(variant(&obj));
	}

	attr[variant("levels")] = variant(&level_vec);

	std::string msg_str = variant(&attr).write_json();
	threading::lock lck(upload_queue_mutex());
	upload_queue.push_back(std::pair<std::string,std::string>("upload-frogatto", msg_str));
}

void send_stats_thread() {
	if(preferences::send_stats() == false) {
		return;
	}

	for(;;) {
		std::vector<std::pair<std::string, std::string> > queue;
		{
			threading::lock lck(upload_queue_mutex());
			if(!send_stats_should_exit && upload_queue.empty()) {
				send_stats_signal().wait_timeout(upload_queue_mutex(), 600000);
			}

			if(send_stats_should_exit && upload_queue.empty()) {
				break;
			}

			queue.swap(upload_queue);
		}

		for(int n = 0; n != queue.size(); ++n) {
			try {
				http_upload(queue[n].second, queue[n].first);
			} catch(...) {
				std::cerr << "ERROR PERFORMING HTTP UPLOAD\n";
			}
		}
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

manager::manager()
#if !TARGET_OS_IPHONE
#if defined(__ANDROID__) && SDL_VERSION_ATLEAST(1, 3, 0)
  : background_thread_("stats-thread", send_stats_thread)
#else
  : background_thread_(send_stats_thread)
#endif
#endif
{}

manager::~manager() {
	send_stats_should_exit = true;
	flush();
}

/*
void prepare_draw(const std::vector<record_ptr>& records)
{
	player_move_record_vertex_array.clear();
	die_record_vertex_array.clear();
	quit_record_vertex_array.clear();

	foreach(const stats::const_record_ptr& record, records) {
		record->prepare_draw();
	}
}
*/

		/*
void draw_points(int r, int g, int b, const std::vector<GLfloat>& v) {
	if(v.empty()) {
		return;
	}

	glPointSize(5);
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4ub(r, g, b, 255);
	glVertexPointer(2, GL_FLOAT, 0, &v[0]);
	glDrawArrays(GL_POINTS, 0, v.size()/2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glColor4ub(255, 255, 255, 255);
}

void draw_stats(const std::vector<record_ptr>& records)
{
	if(!player_move_record_vertex_array.empty()) {
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glColor4ub(0, 0, 255, 128);
		glVertexPointer(2, GL_FLOAT, 0, &player_move_record_vertex_array[0]);
		glDrawArrays(GL_LINES, 0, player_move_record_vertex_array.size()/2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glColor4ub(255, 255, 255, 255);
	}

	draw_points(255, 0, 0, die_record_vertex_array);
	draw_points(255, 255, 0, quit_record_vertex_array);
}
*/
void flush()
{
	send_stats(write_queue);
	threading::lock lck(upload_queue_mutex());
	send_stats_signal().notify_one();
}

entry::entry(const std::string& type) : level_id_(level::current().id())
{
	static const variant TypeStr("type");
	records_[TypeStr] = variant(type);
}

entry::entry(const std::string& type, const std::string& level_id) : level_id_(level_id)
{
	static const variant TypeStr("type");
	records_[TypeStr] = variant(type);
}

entry::~entry()
{
	record(variant(&records_), level_id_);
}

entry& entry::set(const std::string& name, const variant& value)
{
	records_[variant(name)] = value;
	return *this;
}

void entry::add_player_pos()
{
	if(level::current().player()) {
		set("x", variant(level::current().player()->get_entity().midpoint().x));
		set("y", variant(level::current().player()->get_entity().midpoint().y));
	}
}

void record(const variant& value)
{
	write_queue[level::current().id()].push_back(value);
}

void record(const variant& value, const std::string& level_id)
{
	write_queue[level_id].push_back(value);
}

}
