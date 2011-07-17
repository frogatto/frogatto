#ifndef STATS_HPP_INCLUDED
#define STATS_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "geometry.hpp"
#include "thread.hpp"
#include "wml_node_fwd.hpp"

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

class record;

typedef boost::shared_ptr<record> record_ptr;
typedef boost::shared_ptr<const record> const_record_ptr;

class record {
public:
	static record_ptr read(wml::const_node_ptr node);
	virtual ~record();
	virtual const char* id() const = 0;
	virtual wml::node_ptr write() const = 0;
	virtual void prepare_draw() const {}
	virtual void draw() const {}
	virtual point location() const = 0;
};

class die_record : public record {
public:
	explicit die_record(const point& p);
	wml::node_ptr write() const;
	void prepare_draw() const;
	void draw() const;
	const char* id() const { return "die"; }
	point location() const { return p_; }
private:
	point p_;
};

class quit_record : public record {
public:
	explicit quit_record(const point& p);
	wml::node_ptr write() const;
	void prepare_draw() const;
	void draw() const;
	const char* id() const { return "quit"; }
	point location() const { return p_; }
private:
	point p_;
};

class load_level_record : public record {
public:
	explicit load_level_record(int ms);
	wml::node_ptr write() const;
	const char* id() const { return "load"; }
	point location() const { return point(0,0); }
private:
	int ms_;
};

class player_move_record : public record {
public:
	player_move_record(const point& src, const point& dst);
	wml::node_ptr write() const;
	void prepare_draw() const;
	void draw() const;
	const char* id() const { return "move"; }
	point location() const { return src_; }
private:
	point src_, dst_;
};

class custom_record : public record {
public:
	custom_record(const std::string& key, const std::string& value, const point& src);
	wml::node_ptr write() const;
	const char* id() const { return "custom"; }
	point location() const { return src_; }
private:
	const std::string key_, value_;
	point src_;
};

void prepare_draw(const std::vector<record_ptr>& records);
void draw_stats(const std::vector<record_ptr>& records);

void record_event(const std::string& lvl, const_record_ptr r);
void flush();

}

#endif
