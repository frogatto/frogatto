#ifndef PROP_HPP_INCLUDED
#define PROP_HPP_INCLUDED

#include "boost/shared_ptr.hpp"

#include "frame.hpp"
#include "geometry.hpp"
#include "wml_node_fwd.hpp"

class prop;
typedef boost::shared_ptr<prop> prop_ptr;
typedef boost::shared_ptr<const prop> const_prop_ptr;

class prop
{
public:
	static std::vector<const_prop_ptr> all_props();
	static void init(wml::const_node_ptr node);
	static const_prop_ptr get(const std::string& id);
	explicit prop(wml::const_node_ptr node);

	const std::string& id() const { return id_; }
	int zorder() const { return zorder_; }
	const frame& get_frame() const { return frame_; }
	const std::vector<rect>& solid_rects() const { return solid_rects_; }

	struct error {
	};

private:
	std::string id_;
	int zorder_;
	frame frame_;
	std::vector<rect> solid_rects_;
};

class prop_object
{
public:
	prop_object(int x, int y, const std::string& id);
	explicit prop_object(wml::const_node_ptr node);
	void draw() const;
	const rect& area() const { return area_; }
	bool operator<(const prop_object& o) const;

	wml::node_ptr write() const;

	int zorder() const { return type_->zorder(); }
	const std::vector<rect>& solid_rects() const { return solid_rects_; }
private:
	const_prop_ptr type_;
	rect area_;

	void calculate_solid_rects();
	std::vector<rect> solid_rects_;
};

bool operator<(int zorder, const prop_object& o);
bool operator<(const prop_object& o, int zorder);

#endif
