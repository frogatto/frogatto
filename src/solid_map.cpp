#include "asserts.hpp"
#include "foreach.hpp"
#include "solid_map.hpp"
#include "texture.hpp"
#include "wml_node.hpp"

void solid_map::create_object_solid_maps(wml::const_node_ptr node, std::vector<const_solid_map_ptr>& v)
{
	if(!node->has_attr("solid_area")) {
		return;
	}

	rect area(node->attr("solid_area"));
	area = rect(area.x()*2, area.y()*2, area.w()*2, area.h()*2);

	const int legs_height = area.w()/2 + 1;
	if(legs_height < area.h()) {
		rect body(area.x(), area.y(), area.w(), area.h() - legs_height);
		solid_map_ptr body_map(new solid_map);
		body_map->id_ = "body";
		body_map->area_ = body;
		body_map->solid_.resize(body.w()*body.h(), true);
		body_map->calculate_side(0, -1, body_map->top_);
		body_map->calculate_side(-1, 0, body_map->left_);
		body_map->calculate_side(1, 0, body_map->right_);
		body_map->calculate_side(-100000, 0, body_map->all_);
		v.push_back(body_map);
	}

	rect legs(area.x(), area.y2() - legs_height, area.w(), legs_height);
	solid_map_ptr legs_map(new solid_map);
	legs_map->id_ = "legs";
	legs_map->area_ = legs;
	legs_map->solid_.resize(legs.w()*legs.h(), false);
	for(int y = 0; y != legs.h(); ++y) {
		for(int x = y; x < legs.w() - y; ++x) {
			legs_map->set_solid(x, y);
		}
	}

	legs_map->calculate_side(0, 1, legs_map->bottom_);
	legs_map->calculate_side(-1, 0, legs_map->left_);
	legs_map->calculate_side(1, 0, legs_map->right_);
	legs_map->calculate_side(-10000, 0, legs_map->all_);
	v.push_back(legs_map);

	std::cerr << "LEGS: ";
	foreach(const point& p, legs_map->bottom_) {
		std::cerr << p.x << "," << p.y << " ";
	}
	std::cerr << "\n";
}

void solid_map::create_object_platform_maps(wml::const_node_ptr node, std::vector<const_solid_map_ptr>& v)
{
	if(!node->has_attr("platform_area")) {
		return;
	}

	rect area(node->attr("platform_area"));
	area = rect(area.x()*2, area.y()*2, area.w()*2, area.h()*2);

	solid_map_ptr platform(new solid_map);
	platform->id_ = "platform";
	platform->area_ = area;
	platform->solid_.resize(area.w()*area.h(), true);
	platform->calculate_side(0, -1, platform->top_);
	platform->calculate_side(0, 1, platform->bottom_);
	platform->calculate_side(-1, 0, platform->left_);
	platform->calculate_side(1, 0, platform->right_);
	platform->calculate_side(-100000, 0, platform->all_);
	v.push_back(platform);
}
const_solid_map_ptr solid_map::create_from_texture(const graphics::texture& t, const rect& area)
{
	solid_map_ptr solid(new solid_map);
	solid->area_ = rect(0, 0, area.w()*2, area.h()*2);
	solid->solid_.resize(solid->area_.w()*solid->area_.h(), false);
	std::cerr << "SOLID MAP: " << solid->area_ << " {{{\n";
	for(int y = 0; y < solid->area_.h(); ++y) {
		for(int x = 0; x < solid->area_.w(); ++x) {
			bool is_solid = !t.is_alpha(area.x() + x/2, area.y() + y/2);
			if(!is_solid && (y&1) && y < solid->area_.h() - 1 && !t.is_alpha(area.x() + x/2, area.y() + y/2 + 1)) {
				//we are scaling things up by double, so we want to smooth
				//things out. In the bottom half of an empty source pixel, we
				//will set it to solid if the pixel below is solid, and the
				//adjacent horizontal pixel is solid
				if((x&1) && x < solid->area_.w() - 1 && !t.is_alpha(area.x() + x/2 + 1, area.y() + y/2)) {
					is_solid = true;
				} else if(!(x&1) && x > 0 && !t.is_alpha(area.x() + x/2 - 1, area.y() + y/2)) {
					is_solid = true;
				}
			}

			if(is_solid) {
				solid->set_solid(x, y);
			}

			std::cerr << (is_solid ? "1" : "0");
		}

		std::cerr << "\n";
	}
	return solid;
}

bool solid_map::solid_at(int x, int y) const
{
	ASSERT_EQ(solid_.size(), area_.w()*area_.h());

	if(x < 0 || y < 0 || x >= area_.w() || y >= area_.h()) {
		return false;
	}

	return solid_[y*area_.w() + x];
}

const std::vector<point>& solid_map::dir(MOVE_DIRECTION d) const
{
	switch(d) {
	case MOVE_LEFT: return left();
	case MOVE_RIGHT: return right();
	case MOVE_UP: return top();
	case MOVE_DOWN: return bottom();
	case MOVE_NONE: return all();
	default:
		assert(false);
		return all();
	}
}

void solid_map::set_solid(int x, int y, bool value)
{
	ASSERT_EQ(solid_.size(), area_.w()*area_.h());
	if(x < 0 || y < 0 || x >= area_.w() || y >= area_.h()) {
		return;
	}

	solid_[y*area_.w() + x] = value;
}

void solid_map::calculate_side(int xdir, int ydir, std::vector<point>& points) const
{
	for(int y = 0; y < area_.h(); ++y) {
		for(int x = 0; x < area_.w(); ++x) {
			if(solid_at(x, y) && !solid_at(x + xdir, y + ydir)) {
				points.push_back(point(area_.x() + x, area_.y() + y));
			}
		}
	}
}

const_solid_info_ptr solid_info::create_from_solid_maps(const std::vector<const_solid_map_ptr>& solid)
{
	if(solid.empty()) {
		return const_solid_info_ptr();
	} else {
		solid_info* result = new solid_info;
		int x1 = solid.front()->area().x();
		int y1 = solid.front()->area().y();
		int x2 = solid.front()->area().x2();
		int y2 = solid.front()->area().y2();
		foreach(const_solid_map_ptr s, solid) {
			if(s->area().x() < x1) {
				x1 = s->area().x();
			}
			if(s->area().y() < y1) {
				y1 = s->area().y();
			}
			if(s->area().x2() > x2) {
				x2 = s->area().x2();
			}
			if(s->area().y2() > y2) {
				y2 = s->area().y2();
			}
		}

		result->area_ = rect::from_coordinates(x1, y1, x2, y2);
		result->solid_= solid;
		return const_solid_info_ptr(result);
	}
}

const_solid_info_ptr solid_info::create(wml::const_node_ptr node)
{
	std::vector<const_solid_map_ptr> solid;
	solid_map::create_object_solid_maps(node, solid);
	return create_from_solid_maps(solid);
}

const_solid_info_ptr solid_info::create_platform(wml::const_node_ptr node)
{
	std::vector<const_solid_map_ptr> platform;
	solid_map::create_object_platform_maps(node, platform);
	return create_from_solid_maps(platform);
}

const_solid_info_ptr solid_info::create_from_texture(const graphics::texture& t, const rect& area)
{
	std::vector<const_solid_map_ptr> solid;
	solid.push_back(solid_map::create_from_texture(t, area));
	return create_from_solid_maps(solid);
}

bool solid_info::solid_at(int x, int y, const std::string** area_id) const
{
	foreach(const const_solid_map_ptr& s, solid_) {
		if(s->solid_at(x - s->area().x(), y - s->area().y())) {
			if(area_id) {
				*area_id = &s->id();
			}
			return true;
		}
	}

	return false;
}
