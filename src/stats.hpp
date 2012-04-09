#ifndef STATS_HPP_INCLUDED
#define STATS_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "geometry.hpp"
#include "thread.hpp"
#include "variant.hpp"

#include <string>

void http_upload(const std::string& payload, const std::string& script);

namespace stats {

//download stats for a given level.
bool download(const std::string& lvl);

class manager {
public:
	manager();
	~manager();
private:
	//currently the stats thread (and thus stats) are disabled, due to
	//causing crashes on the iPhone. Need to investigate.
#if !TARGET_OS_IPHONE
	threading::thread background_thread_;
#endif
};

class entry {
public:
	explicit entry(const std::string& type);
	entry(const std::string& type, const std::string& level_id);
	~entry();
	entry& set(const std::string& name, const variant& value);
	void add_player_pos();
private:
	entry(const entry& o);
	void operator=(const entry& o) const;

	std::string level_id_;
	std::map<variant, variant> records_;
};

void record(const variant& value);
void record(const variant& value, const std::string& level_id);

void flush();

}

#endif
