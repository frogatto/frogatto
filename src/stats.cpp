#include "graphics.hpp"
#include <map>
#include <sstream>
#include <stdio.h>
#include <vector>

#include <boost/array.hpp>
#if !defined(__native_client__)
#include <boost/asio.hpp>
#endif
#include <boost/bind.hpp>

#include "checksum.hpp"
#include "filesystem.hpp"
#include "formatter.hpp"
#if !defined(__native_client__)
#include "http_client.hpp"
#endif
#include "level.hpp"
#include "module.hpp"
#include "preferences.hpp"
#include "playable_custom_object.hpp"
#include "stats.hpp"

namespace {
std::string get_stats_dir() {
	return sys::get_dir(std::string(preferences::user_data_path()) + "stats/") + "/";
}

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
	attr[variant("module")] = variant(module::get_module_name());
	attr[variant("module_version")] = variant(module::get_module_version());
	attr[variant("user_id")] = variant(preferences::get_unique_user_id());

	if(checksum::is_verified()) {
		attr[variant("signature")] = variant(checksum::game_signature());
	}

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

namespace {
void finish_upload(std::string response, bool* flag)
{
	std::cerr << "UPLOAD COMPLETE: " << response << "\n";
	*flag = true;
}

void upload_progress(int sent, int total, bool uploaded)
{
	std::cerr << "SENT " << sent << "/" << total << "\n";
}

}

void send_stats_thread() {
	if(preferences::send_stats() == false) {
		return;
	}

#if !defined(__native_client__)
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

		bool done = false;
		for(int n = 0; n != queue.size(); ++n) {
			http_client client("theargentlark.com", "5000");
			client.send_request("POST /cgi-bin/" + queue[n].first, 
				queue[n].second, 
				boost::bind(finish_upload, _1, &done),
				boost::bind(finish_upload, _1, &done),
				boost::bind(upload_progress, _1, _2, _3));				
			while(!done) {
				client.process();
			}
		}
	}
#endif
}

}

void download_finish(std::string stats_wml, bool* flag, const std::string& lvl)
{
	sys::write_file(get_stats_dir() + lvl, stats_wml);
	std::cerr << "DOWNLOAD COMPLETE\n";
	*flag = true;
}

void download_error(std::string response, bool* flag, bool* err)
{
	std::cerr << "DOWNLOAD ERROR: " << response << "\n";
	*flag = true;
	*err = true;
}

void download_progress(int sent, int total, bool uploaded)
{
	std::cerr << "SENT " << sent << "/" << total << "\n";
}

bool download(const std::string& lvl) {
#if !defined(__native_client__)
	bool done = false;
	bool err = false;
	http_client client("www.wesnoth.org", "80");
	client.send_request("GET /files/dave/frogatto-stats/" + lvl, 
		"", 
		boost::bind(download_finish, _1, &done, lvl),
		boost::bind(download_error, _1, &done, &err),
		boost::bind(download_progress, _1, _2, _3));				
	while(!done) {
		client.process();
	}
	return !err;
#else
	return false;
#endif
}

namespace {
threading::thread* background_thread = NULL;
}

manager::manager()
{
#if !TARGET_OS_IPHONE
	if(!background_thread) {
#if defined(__ANDROID__) && SDL_VERSION_ATLEAST(1, 3, 0)
		background_thread = new threading::thread("stats-thread", send_stats_thread);
#else
		background_thread = new threading::thread(send_stats_thread);
#endif
	}
#endif
}

manager::~manager() {
	flush_and_quit();
}

void flush_and_quit() {
	if(background_thread) {
		send_stats_should_exit = true;
		flush();

		delete background_thread;
		background_thread = NULL;
	}
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

entry& entry::add_player_pos()
{
	if(level::current().player()) {
		set("x", variant(level::current().player()->get_entity().midpoint().x));
		set("y", variant(level::current().player()->get_entity().midpoint().y));
	}

	return *this;
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
