#ifndef LIGHT_HPP_INCLUDED
#define LIGHT_HPP_INCLUDED

#include "formula_callable.hpp"
#include "geometry.hpp"
#include "wml_node_fwd.hpp"

#include <boost/intrusive_ptr.hpp>

class custom_object;
class light;

typedef boost::intrusive_ptr<light> light_ptr;
typedef boost::intrusive_ptr<const light> const_light_ptr;

class light : public game_logic::formula_callable
{
public:
	static light_ptr create_light(const custom_object& obj, wml::const_node_ptr node);

	virtual wml::node_ptr write() const = 0;

	explicit light(const custom_object& obj);
	virtual ~light();
	virtual void process() = 0;
	virtual bool on_screen(const rect& screen_area) const = 0;
	virtual void draw(const rect& screen_area, const unsigned char* color) const = 0;
protected:
	const custom_object& object() const { return obj_; }
private:
	virtual variant get_value(const std::string& key) const;
	const custom_object& obj_;
};

class circle_light : public light
{
public:
	circle_light(const custom_object& obj, wml::const_node_ptr node);
	circle_light(const custom_object& obj, int radius);
	wml::node_ptr write() const;
	void process();
	bool on_screen(const rect& screen_area) const;
	void draw(const rect& screen_area, const unsigned char* color) const;
private:
	point center_;
	int radius_;
};

class light_fade_length_setter
{
	int old_value_;
public:
	explicit light_fade_length_setter(int value);
	~light_fade_length_setter();
};

#endif
