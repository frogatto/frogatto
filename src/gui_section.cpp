#include <map>

#include "asserts.hpp"
#include "foreach.hpp"
#include "gui_section.hpp"
#include "raster.hpp"
#include "string_utils.hpp"
#include "variant.hpp"

namespace {
typedef std::map<std::string, const_gui_section_ptr> cache_map;
cache_map cache;
}

void gui_section::init(variant node)
{
	foreach(const variant& section_node, node["section"].as_list()) {
		const std::string& id = section_node["id"].as_string();
		cache[id].reset(new gui_section(section_node));
	}
}

const_gui_section_ptr gui_section::get(const variant& v)
{
	if(v.has_key("name")) {
		return get(v["name"].as_string());
	} else {
		const std::string& id = v["id"].as_string();
		cache[id].reset(new gui_section(v));
		return cache[id];
	}
}

const_gui_section_ptr gui_section::get(const std::string& key)
{
	cache_map::const_iterator itor = cache.find(key);
	if(itor == cache.end()) {
		ASSERT_LOG(false, "GUI section " << key << " not found in cache");
		return const_gui_section_ptr();
	}

	return itor->second;
}

gui_section::gui_section(variant node)
  : texture_(graphics::texture::get(node["image"].as_string())),
    area_(node["rect"]),
	x_adjust_(0), y_adjust_(0), x2_adjust_(0), y2_adjust_(0)
{
	draw_area_ = area_;

	if(node.has_key("frame_info")) {
		std::vector<int> buf = node["frame_info"].as_list_int();
		if(buf.size() == 8) {
			x_adjust_ = buf[0];
			y_adjust_ = buf[1];
			x2_adjust_ = buf[2];
			y2_adjust_ = buf[3];
			draw_area_ = rect(buf[4], buf[5], buf[6], buf[7]);
		}
	}
}

void gui_section::blit(int x, int y, int w, int h) const
{
	const int scale = w/area_.w();

	const GLfloat TextureEpsilon = 0.1;
	graphics::blit_texture(texture_, x+x_adjust_*scale, y+y_adjust_*scale, w - x_adjust_*scale - x2_adjust_*scale, h - y_adjust_*scale - y2_adjust_*scale, 0.0,
	                       GLfloat(draw_area_.x()+TextureEpsilon)/GLfloat(texture_.width()),
	                       GLfloat(draw_area_.y()+TextureEpsilon)/GLfloat(texture_.height()),
	                       GLfloat(draw_area_.x2()-TextureEpsilon)/GLfloat(texture_.width()),
	                       GLfloat(draw_area_.y2()-TextureEpsilon)/GLfloat(texture_.height()));
}
