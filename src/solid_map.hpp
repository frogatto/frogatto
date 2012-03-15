#ifndef SOLID_MAP_HPP_INCLUDED
#define SOLID_MAP_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include <vector>

#include "geometry.hpp"
#include "solid_map_fwd.hpp"
#include "wml_node_fwd.hpp"

enum MOVE_DIRECTION { MOVE_LEFT, MOVE_RIGHT, MOVE_UP, MOVE_DOWN, MOVE_NONE };

namespace graphics {
class texture;
}

class solid_map
{
public:
	static void create_object_solid_maps(wml::const_node_ptr node, std::vector<const_solid_map_ptr>& v);
	static void create_object_platform_maps(const rect& area, std::vector<const_solid_map_ptr>& v);
	static solid_map_ptr create_from_texture(const graphics::texture& t, const rect& area);

	const std::string& id() const { return id_; }
	const rect& area() const { return area_; }

	bool solid_at(int x, int y) const;

	const std::vector<point>& dir(MOVE_DIRECTION d) const;
	const std::vector<point>& left() const { return left_; }
	const std::vector<point>& right() const { return right_; }
	const std::vector<point>& top() const { return top_; }
	const std::vector<point>& bottom() const { return bottom_; }
	const std::vector<point>& all() const { return all_; }
private:
	static const_solid_map_ptr create_object_solid_map_from_solid_node(wml::const_node_ptr node);

	solid_map() {}

	void set_solid(int x, int y, bool value=true);

	void calculate_side(int xdir, int ydir, std::vector<point>& points) const;

	void apply_offsets(const std::vector<int>& offsets);

	std::string id_;
	rect area_;

	std::vector<bool> solid_;

	//all the solid points that are on the different sides of the solid area.
	std::vector<point> left_, right_, top_, bottom_, all_;
};

class solid_info
{
public:
	static const_solid_info_ptr create(wml::const_node_ptr node);
	static const_solid_info_ptr create_platform(wml::const_node_ptr node);
	static const_solid_info_ptr create_platform(const rect& area);
	static const_solid_info_ptr create_from_texture(const graphics::texture& t, const rect& area);
	const std::vector<const_solid_map_ptr>& solid() const { return solid_; }
	const rect& area() const { return area_; }
	bool solid_at(int x, int y, const std::string** area_id=NULL) const;
private:
	static const_solid_info_ptr create_from_solid_maps(const std::vector<const_solid_map_ptr>& v);

	std::vector<const_solid_map_ptr> solid_;
	rect area_;
};

#endif
