#include <map>

#include "foreach.hpp"
#include "graphical_font.hpp"
#include "raster.hpp"
#include "variant_utils.hpp"

namespace {
typedef std::map<std::string, graphical_font_ptr> cache_map;
cache_map cache;

unsigned int utf8_to_codepoint(std::string::const_iterator& i, std::string::const_iterator end) {
	unsigned int codepoint = 0;

	if((*i & 0xc0) == 0x80) {
		//*i is an unexpected following byte
		return 0;
	}
	if((*i & 0xc0) == 0xc0) {
		//*i is the leading byte of an UTF-8 encoded multi-byte character.
		if ((*i & 0xe0) == 0xc0) {
			//two byte sequence: 110xxxyy 10yyyyyy
			codepoint = (*i & 0x1f) << 6;
			if (++i == end) {
				return 0;
			}
			if ((*i & 0xc0) == 0x80) {
				codepoint |= *i & 0x3f;
			} else {
				codepoint = 0xfffd; // U+FFFD is the replacement character
			}
		} else if ((*i & 0xf0) == 0xe0) {
			//three byte sequence: 1110xxxx 10xxxxyy 10yyyyyy
			codepoint = (*i & 0x0f) << 12;
			if (++i == end) {
				return 0;
			}
			if ((*i & 0xc0) == 0x80) {
				codepoint |= (*i & 0x3f) << 6;
				if (++i == end) {
					return 0;
				}
				if ((*i & 0xc0) == 0x80) {
					codepoint |= *i & 0x3f;
				} else {
					codepoint = 0xfffd;
				}
			} else {
				codepoint = 0xfffd;
			}
		} else if ((*i & 0xf8) == 0xf0) {
			//four byte sequence: 11110xxx 10xxyyyy 10yyyyzz 10zzzzzz
			codepoint = (*i & 0x07) << 18;
			if (++i == end) {
				return 0;
			}
			if ((*i & 0xc0) == 0x80) {
				codepoint |= (*i & 0x3f) << 12;
				if (++i == end) {
					return 0;
				}
				if ((*i & 0xc0) == 0x80) {
					codepoint |= (*i & 0x3f) << 6;
					if (++i == end) {
						return 0;
					}
					if ((*i & 0xc0) == 0x80) {
						codepoint |= *i & 0x3f;
					} else {
						codepoint = 0xfffd;
					}
				} else {
					codepoint = 0xfffd;
				}
			} else {
				codepoint = 0xfffd;
			}
		}
	} else {
		//c is an ASCII character
		codepoint = *i;
	}
	return codepoint;	
}
}

void graphical_font::init(variant node)
{
	foreach(const variant& font_node, node["font"].as_list()) {
		graphical_font_ptr font(new graphical_font(font_node));
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

graphical_font::graphical_font(variant node)
  : id_(node["id"].as_string()), texture_(graphics::texture::get(node["texture"].as_string())),
    kerning_(node["kerning"].as_int(2))
{
	int pad = 2;
	if (node.has_key("pad")){
		pad = node["pad"].as_int(2);
	}
	
	rect current_rect;
	foreach(const variant& char_node, node["chars"].as_list()) {
		if(char_node.has_key("pad")) {
			pad = char_node["pad"].as_int();
		}

		const std::string& chars = char_node["chars"].as_string();
		if(char_node.has_key("width")) {
			current_rect = rect(current_rect.x(), current_rect.y(),
			                    char_node["width"].as_int(),
			                    current_rect.h());
		} else {
			current_rect = rect(char_node["rect"].as_list_int());
		}
		for(std::string::const_iterator i = chars.begin(); i != chars.end(); ++i) {
			unsigned int codepoint = utf8_to_codepoint(i, chars.end());
			if (codepoint == 0)
				break;

			char_rect_map_[codepoint] = current_rect;

			current_rect = rect(current_rect.x() + current_rect.w() + pad,
			                    current_rect.y(),
			                    current_rect.w(), current_rect.h());
		}
	}
}

rect graphical_font::draw(int x, int y, const std::string& text, int size) const
{
	return do_draw(x, y, text, true, size);
}

namespace {
std::vector<GLfloat> font_varray;
std::vector<GLfloat> font_tcarray;
}

rect graphical_font::do_draw(int x, int y, const std::string& text, bool draw_text, int size) const
{
	if(text.empty()) {
		return rect(x, y, 0, 0);
	}

	if(draw_text) {
		texture_.set_as_current_texture();
	}

	font_varray.clear();
	font_tcarray.clear();

	int x2 = x, y2 = y;
	int xpos = x, ypos = y, highest = 0;
	for(std::string::const_iterator i = text.begin(); i != text.end(); ++i) {
		if(*i == '\n') {
			ypos = ypos + ((highest+4)*size)/2;
			xpos = x;
			highest = 0;
			continue;
		}

		unsigned int codepoint = utf8_to_codepoint(i, text.end());
		if (codepoint == 0)
			break;

		char_rect_map::const_iterator it = char_rect_map_.find (codepoint);
		if (it == char_rect_map_.end())
			continue;

		const rect& r = it->second;

		if(draw_text) {
			const GLfloat TextureEpsilon = 0.1f;
			const GLfloat u1 = graphics::texture::get_coord_x(GLfloat(r.x() + TextureEpsilon)/GLfloat(texture_.width()));
			const GLfloat v1 = graphics::texture::get_coord_y(GLfloat(r.y() + TextureEpsilon)/GLfloat(texture_.height()));
			const GLfloat u2 = graphics::texture::get_coord_x(GLfloat(r.x2() - TextureEpsilon)/GLfloat(texture_.width()));
			const GLfloat v2 = graphics::texture::get_coord_y(GLfloat(r.y2() - TextureEpsilon)/GLfloat(texture_.height()));

			const int x = xpos&preferences::xypos_draw_mask;
			const int y = ypos&preferences::xypos_draw_mask;

			font_varray.push_back(GLfloat(x));
			font_varray.push_back(GLfloat(y));
			font_varray.push_back(GLfloat(x));
			font_varray.push_back(GLfloat(y));
			font_varray.push_back(GLfloat(x));
			font_varray.push_back(GLfloat(y + (r.h())*size));
			font_tcarray.push_back(u1);
			font_tcarray.push_back(v1);
			font_tcarray.push_back(u1);
			font_tcarray.push_back(v1);
			font_tcarray.push_back(u1);
			font_tcarray.push_back(v2);

			font_varray.push_back(GLfloat(x + (r.w())*size));
			font_varray.push_back(GLfloat(y));
			font_varray.push_back(GLfloat(x + (r.w())*size));
			font_varray.push_back(GLfloat(y + (r.h())*size));
			font_varray.push_back(GLfloat(x + (r.w())*size));
			font_varray.push_back(GLfloat(y + (r.h())*size));
			font_tcarray.push_back(u2);
			font_tcarray.push_back(v1);
			font_tcarray.push_back(u2);
			font_tcarray.push_back(v2);
			font_tcarray.push_back(u2);
			font_tcarray.push_back(v2);
		}

		if(ypos + r.h()*size > y2) {
			y2 = ypos + r.h()*size;
		}
		
		xpos += r.w()*size + kerning_*size;

		if(xpos > x2) {
			x2 = xpos;
		}

		if(r.h() > highest) {
			highest = r.h();
		}
	}

	if(draw_text && !font_varray.empty()) {
		texture_.set_as_current_texture();
#if defined(USE_GLES2)
		gles2::active_shader()->prepare_draw();
		gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &font_varray.front());
		gles2::active_shader()->shader()->texture_array(2, GL_FLOAT, 0, 0, &font_tcarray.front());
		
#else
		glVertexPointer(2, GL_FLOAT, 0, &font_varray.front());
		glTexCoordPointer(2, GL_FLOAT, 0, &font_tcarray.front());
#endif
		glDrawArrays(GL_TRIANGLE_STRIP, 0, font_varray.size()/2);
	}

	return rect(x, y, x2 - x, y2 - y);
}

rect graphical_font::dimensions(const std::string& text, int size) const
{
	return do_draw(0, 0, text, false, size);
}

