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
  : id_(node->attr("id")), texture_(graphics::texture::get(node->attr("texture"))),
    kerning_(wml::get_int(node, "kerning", 2))
{
	int pad = 2;
	if (node->has_attr("pad")){
		pad = wml::get_int(node, "pad", 2);
	}
	
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

namespace {
std::vector<GLfloat> font_varray;
std::vector<GLfloat> font_tcarray;
}

rect graphical_font::do_draw(int x, int y, const std::string& text, bool draw_text) const
{
	if(draw_text) {
		texture_.set_as_current_texture();
	}

	font_varray.clear();
	font_tcarray.clear();

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

		if(draw_text) {
			const GLfloat u1 = graphics::texture::get_coord_x(GLfloat(r.x ())/GLfloat(texture_.width()));
			const GLfloat v1 = graphics::texture::get_coord_y(GLfloat(r.y ())/GLfloat(texture_.height()));
			const GLfloat u2 = graphics::texture::get_coord_x(GLfloat(r.x2() + kerning_)/GLfloat(texture_.width()));
			const GLfloat v2 = graphics::texture::get_coord_y(GLfloat(r.y2() + kerning_)/GLfloat(texture_.height()));

			font_varray.push_back(xpos);
			font_varray.push_back(ypos);
			font_varray.push_back(xpos);
			font_varray.push_back(ypos + (r.h() + kerning_)*2);
			font_tcarray.push_back(u1);
			font_tcarray.push_back(v1);
			font_tcarray.push_back(u1);
			font_tcarray.push_back(v2);

			font_varray.push_back(xpos + (r.w() + kerning_)*2);
			font_varray.push_back(ypos);
			font_varray.push_back(xpos + (r.w() + kerning_)*2);
			font_varray.push_back(ypos + (r.h() + kerning_)*2);
			font_tcarray.push_back(u2);
			font_tcarray.push_back(v1);
			font_tcarray.push_back(u2);
			font_tcarray.push_back(v2);
		}

		if(xpos + r.w()*2 > x2) {
			x2 = xpos + r.w()*2;
		}

		if(ypos + r.h()*2 > y2) {
			y2 = ypos + r.h()*2;
		}
		
		xpos += r.w()*2 + kerning_*2;
		if(r.h() > highest) {
			highest = r.h();
		}
	}

	if(draw_text) {
		texture_.set_as_current_texture();
		glVertexPointer(2, GL_FLOAT, 0, &font_varray.front());
		glTexCoordPointer(2, GL_FLOAT, 0, &font_tcarray.front());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, font_varray.size()/2);
	}

	return rect(x, y, x2 - x, y2 - y);
}

rect graphical_font::dimensions(const std::string& text) const
{
	return do_draw(0, 0, text, false);
}

