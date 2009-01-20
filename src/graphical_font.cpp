#include <map>

#include "graphical_font.hpp"
#include "raster.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
typedef std::map<std::string, graphical_font_ptr> cache_map;
cache_map cache;
}

void graphical_font::init(wml::const_node_ptr node)
{
	wml::node::const_child_iterator i1 = node->begin_child("font");
	wml::node::const_child_iterator i2 = node->end_child("font");
	for(; i1 != i2; ++i1) {
		graphical_font_ptr font(new graphical_font(i1->second));
		cache[font->id()] = font;
	}
}

const_graphical_font_ptr graphical_font::get(const std::string& id)
{
	cache_map::const_iterator itor = cache.find(id);
	if(itor == cache.end()) {
		return const_graphical_font_ptr();
	}

	return itor->second;
}

graphical_font::graphical_font(wml::const_node_ptr node)
  : id_(node->attr("id")), texture_(graphics::texture::get(node->attr("texture")))
{
	int pad = 2;
	wml::node::const_child_iterator i1 = node->begin_child("chars");
	wml::node::const_child_iterator i2 = node->end_child("chars");
	rect current_rect;
	for(; i1 != i2; ++i1) {
		if(i1->second->has_attr("pad")) {
			pad = wml::get_int(i1->second, "pad");
		}

		const std::string& chars = i1->second->attr("chars");
		if(i1->second->has_attr("width")) {
			current_rect = rect(current_rect.x(), current_rect.y(),
			                    wml::get_int(i1->second, "width"),
			                    current_rect.h());
		} else {
			current_rect = rect(i1->second->attr("rect"));
		}
		for(std::string::const_iterator i = chars.begin(); i != chars.end(); ++i) {
			const unsigned char c = *i;
			if(char_rect_map_.size() <= c) {
				char_rect_map_.resize(c + 1);
			}

			char_rect_map_[c] = current_rect;

			current_rect = rect(current_rect.x() + current_rect.w() + pad,
			                    current_rect.y(),
			                    current_rect.w(), current_rect.h());
		}
	}
}

rect graphical_font::draw(int x, int y, const std::string& text) const
{
	return do_draw(x, y, text);
}

rect graphical_font::do_draw(int x, int y, const std::string& text, bool draw_text) const
{
	int x2 = x, y2 = y;
	int xpos = x, ypos = y, highest = 0;
	for(std::string::const_iterator i = text.begin(); i != text.end(); ++i) {
		if(*i == '\n') {
			ypos = y + highest;
			xpos = x;
			highest = 0;
			continue;
		}

		const unsigned char c = *i;
		if(c >= char_rect_map_.size() || char_rect_map_[c].w() == 0) {
			continue;
		}

		const rect& r = char_rect_map_[c];

		if(draw_text && *i != ' ') {
			graphics::blit_texture(texture_, xpos, ypos, r.w()*2, r.h()*2, 0.0,
			                GLfloat(r.x ())/GLfloat(texture_.width()),
			                GLfloat(r.y ())/GLfloat(texture_.height()),
			                GLfloat(r.x2())/GLfloat(texture_.width()),
			                GLfloat(r.y2())/GLfloat(texture_.height()));
		}

		if(xpos + r.w()*2 > x2) {
			x2 = xpos + r.w()*2;
		}

		if(ypos + r.h()*2 > y2) {
			y2 = ypos + r.h()*2;
		}
		
		xpos += r.w()*2 + 4;
		if(r.h() > highest) {
			highest = r.h();
		}
	}

	return rect(x, y, x2 - x, y2 - y);
}

rect graphical_font::dimensions(const std::string& text) const
{
	return do_draw(0, 0, text, false);
}

