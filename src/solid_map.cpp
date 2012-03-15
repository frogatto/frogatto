#include "asserts.hpp"
#include "foreach.hpp"
#include "solid_map.hpp"
#include "string_utils.hpp"
#include "texture.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

const_solid_map_ptr solid_map::create_object_solid_map_from_solid_node(wml::const_node_ptr node)
{
	solid_map_ptr result(create_from_texture(graphics::texture::get(node->attr("image")), rect(node->attr("area"))));
	result->id_ = node->attr("id");
	return result;

}

void solid_map::create_object_solid_maps(wml::const_node_ptr node, std::vector<const_solid_map_ptr>& v)
{
	FOREACH_WML_CHILD(solid_node, node, "solid") {
		v.push_back(create_object_solid_map_from_solid_node(solid_node));
	}

	if(!node->has_attr("solid_area") || node->attr("solid_area").str() == "none") {
		return;
	}

	rect area(node->attr("solid_area"));
	area = rect(area.x()*2, area.y()*2, area.w()*2, area.h()*2);

	const int feet_width = wml::get_int(node, "feet_width", 0);

	int legs_height = area.w()/2 + 1 - feet_width;
	if(wml::get_bool(node, "has_feet", true) == false || node->has_attr("solid_offsets") || node->attr("solid_shape").str() == "rect" || legs_height < 0) {
		legs_height = 0;
	}


	//flat is a special case which says the solid area is to be
	//precisely one pixel high.
	if(node->attr("solid_shape").str() == "flat") {
		legs_height = 0;
		area = rect(area.x(), area.y()+area.h()-1, area.w(), 1);
	}

	if(legs_height < area.h()) {
		rect body(area.x(), area.y(), area.w(), area.h() - legs_height);
		solid_map_ptr body_map(new solid_map);
		body_map->id_ = "body";
		body_map->area_ = body;
		body_map->solid_.resize(body.w()*body.h(), true);
		if(node->has_attr("solid_offsets")) {
			body_map->apply_offsets(wml::get_vector_int(node, "solid_offsets"));
		}

		body_map->calculate_side(0, -1, body_map->top_);
		body_map->calculate_side(-1, 0, body_map->left_);
		body_map->calculate_side(1, 0, body_map->right_);
		body_map->calculate_side(-100000, 0, body_map->all_);

		if(legs_height == 0) {
			body_map->calculate_side(0, 1, body_map->bottom_);
		}
		v.push_back(body_map);
	} else {
		legs_height = area.h();
	}

	if(legs_height) {
		//std::cerr << "LEGS_HEIGHT: " << node->attr("id") << " " << legs_height << ", FEET_WIDTH: " << feet_width << "\n";
		rect legs(area.x(), area.y2() - legs_height, area.w(), legs_height);
		solid_map_ptr legs_map(new solid_map);
		legs_map->id_ = "legs";
		legs_map->area_ = legs;
		legs_map->solid_.resize(legs.w()*legs.h(), false);
		for(int y = 0; y < legs.h()-1; ++y) {
			for(int x = y; x < legs.w() - y; ++x) {
				//std::cerr << "LEGS_SOLID: " << x << ", " << y << "\n";
				legs_map->set_solid(x, y);
			}
		}

		legs_map->calculate_side(0, 1, legs_map->bottom_);
		legs_map->calculate_side(-1, 0, legs_map->left_);
		legs_map->calculate_side(1, 0, legs_map->right_);
		legs_map->calculate_side(-10000, 0, legs_map->all_);
		v.push_back(legs_map);
	}
}

void solid_map::create_object_platform_maps(const rect& area_ref, std::vector<const_solid_map_ptr>& v)
{

	//intentionally do NOT double the height of the area.
	rect area(area_ref.x()*2, area_ref.y()*2, area_ref.w()*2, 1);

	ASSERT_EQ(area.h(), 1);

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
solid_map_ptr solid_map::create_from_texture(const graphics::texture& t, const rect& area_rect)
{
	rect area = area_rect;

	bool found_solid = false;
	while(!found_solid && area.h() > 0) {
		for(int x = 0; x < area.w(); ++x) {
			if(!t.is_alpha(area.x() + x, area.y() + area.h() - 1)) {
				found_solid = true;
				break;
			}
		}

		if(!found_solid) {
			area = rect(area.x(), area.y(), area.w(), area.h()-1);
		}
	}

	found_solid = false;
	while(!found_solid && area.h() > 0) {
		for(int x = 0; x < area.w(); ++x) {
			if(!t.is_alpha(area.x() + x, area.y())) {
				found_solid = true;
				break;
			}
		}

		if(!found_solid) {
			area = rect(area.x(), area.y()+1, area.w(), area.h()-1);
		}
	}

	found_solid = false;
	while(!found_solid && area.w() > 0) {
		for(int y = 0; y < area.h(); ++y) {
			if(!t.is_alpha(area.x(), area.y() + y)) {
				found_solid = true;
				break;
			}
		}

		if(!found_solid) {
			area = rect(area.x()+1, area.y(), area.w()-1, area.h());
		}
	}

	found_solid = false;
	while(!found_solid && area.w() > 0) {
		for(int y = 0; y < area.h(); ++y) {
			if(!t.is_alpha(area.x() + area.w() - 1, area.y() + y)) {
				found_solid = true;
				break;
			}
		}

		if(!found_solid) {
			area = rect(area.x(), area.y(), area.w()-1, area.h());
		}
	}

	solid_map_ptr solid(new solid_map);
	solid->area_ = rect(area.x()*2, area.y()*2, area.w()*2, area.h()*2);
	solid->solid_.resize(solid->area_.w()*solid->area_.h(), false);
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
		}
	}
	return solid;
}

bool solid_map::solid_at(int x, int y) const
{
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

void solid_map::apply_offsets(const std::vector<int>& offsets)
{
	if(offsets.size() <= 1) {
		return;
	}

	const int seg_width = (area_.w()*1024)/(offsets.size()-1);
	for(int x = 0; x != area_.w(); ++x) {
		const int pos = x*1024;
		const int segment = pos/seg_width;
		ASSERT_GE(segment, 0);
		ASSERT_LT(segment, offsets.size()-1);

		const int partial = pos%seg_width;
		const int offset = (partial*offsets[segment+1]*2 + (seg_width-partial)*offsets[segment]*2)/seg_width;

		for(int y = 0; y < offset; ++y) {
			set_solid(x, y, false);
		}
	}
}

void solid_map::calculate_side(int xdir, int ydir, std::vector<point>& points) const
{
	int index = 0;
	const int height = area_.h();
	const int width = area_.w();
	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < width; ++x) {
			//for performance reasons, check our current position directly
			//rather than calling solid_at() so we don't do bounds checking.
			if(solid_[index] && !solid_at(x + xdir, y + ydir)) {
				points.push_back(point(area_.x() + x, area_.y() + y));
			}

			++index;
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

		result->area_ = rect::from_coordinates(x1, y1, x2-1, y2-1);
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

	if(!node->has_attr("platform_area")) {
		return const_solid_info_ptr();
	}

	std::vector<std::string> v = util::split(node->attr("platform_area"), '|');
	foreach(const std::string& area, v) {
		solid_map::create_object_platform_maps(rect(area), platform);
	}

	return create_from_solid_maps(platform);
}

const_solid_info_ptr solid_info::create_platform(const rect& area)
{
	std::vector<const_solid_map_ptr> platform;
	solid_map::create_object_platform_maps(area, platform);
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
