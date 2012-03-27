#ifndef POWERUP_HPP_INCLUDED
#define POWERUP_HPP_INCLUDED

#include <boost/scoped_ptr.hpp>

#include <string>

#include "frame.hpp"
#include "powerup_fwd.hpp"
#include "variant.hpp"
#include "wml_modify.hpp"

class frame;

class powerup
{
public:
	explicit powerup(variant node);
	static void init(variant node);
	static const_powerup_ptr get(const std::string& id);

	const std::string& id() const { return id_; }
	const wml::modifier& modifier() const { return modifier_; }
	const frame& icon() const { return *icon_; }
	int duration() const { return duration_; }
	bool is_permanent() const { return permanent_; }

private:
	std::string id_;
	wml::modifier modifier_;
	boost::scoped_ptr<frame> icon_;
	int duration_;
	bool permanent_;
};

#endif
