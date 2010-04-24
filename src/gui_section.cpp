#include <map>

#include "gui_section.hpp"
#include "raster.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
typedef std::map<std::string, const_gui_section_ptr> cache_map;
cache_map cache;
}

void gui_section::init(wml::const_node_ptr node)
{
	wml::node::const_child_iterator i1 = node->begin_child("section");
	wml::node::const_child_iterator i2 = node->end_child("section");
	for(; i1 != i2; ++i1) {
		const std::string& id = i1->second->attr("id");
		cache[id].reset(new gui_section(i1->second));
	}
}

const_gui_section_ptr gui_section::get(const std::string& key)
{
	cache_map::const_iterator itor = cache.find(key);
	if(itor == cache.end()) {
		assert(false); //TODO: replace with an exception.
		return const_gui_section_ptr();
	}

	return itor->second;
}

gui_section::gui_section(wml::const_node_ptr node)
  : texture_(graphics::texture::get(node->attr("image"))),
    area_(node->attr("rect"))
{
}

void gui_section::blit(int x, int y, int w, int h) const
{
	const GLfloat TextureEpsilon = 0.1;
	graphics::blit_texture(texture_, x, y, w, h, 0.0,
	                       GLfloat(area_.x()+TextureEpsilon)/GLfloat(texture_.width()),
	                       GLfloat(area_.y()+TextureEpsilon)/GLfloat(texture_.height()),
	                       GLfloat(area_.x2()-TextureEpsilon)/GLfloat(texture_.width()),
	                       GLfloat(area_.y2()-TextureEpsilon)/GLfloat(texture_.height()));
}
