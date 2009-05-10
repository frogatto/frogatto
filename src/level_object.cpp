#include <iostream>
#include <map>
#include <string>

#include "draw_tile.hpp"
#include "level_object.hpp"
#include "raster.hpp"
#include "string_utils.hpp"
#include "surface.hpp"
#include "surface_cache.hpp"
#include "wml_utils.hpp"

namespace {
typedef std::map<std::string,const_level_object_ptr> tiles_map;
tiles_map tiles_cache;
}

bool level_tile::is_solid(int xpos, int ypos) const
{
	return object->is_solid(face_right ? (xpos - x) : (x + object->width() - xpos - 1), ypos - y);
}

std::vector<const_level_object_ptr> level_object::all()
{
	std::vector<const_level_object_ptr> res;
	for(tiles_map::const_iterator i = tiles_cache.begin(); i != tiles_cache.end(); ++i) {
		res.push_back(i->second);
	}

	return res;
}

level_tile level_object::build_tile(wml::const_node_ptr node)
{
	level_tile res;
	res.x = wml::get_int(node, "x");
	res.y = wml::get_int(node, "y");
	res.zorder = wml::get_int(node, "zorder");
	if(tiles_cache.count(node->attr("tile"))) {
		res.object = tiles_cache[node->attr("tile")];
	}
	res.face_right = wml::get_bool(node, "face_right");
	res.rotate = wml::get_int(node, "rotate", 0);
	return res;
}

void level_object::init(wml::const_node_ptr node)
{
	wml::node::const_child_iterator t1 = node->begin_child("tile");
	wml::node::const_child_iterator t2 = node->end_child("tile");
	for(; t1 != t2; ++t1) {
		tiles_cache[t1->second->attr("id")].reset(new level_object(t1->second));
	}
}

level_object::level_object(wml::const_node_ptr node)
  : id_(node->attr("id")), t_(graphics::texture::get(node->attr("image"))),
    width_(-1),
	all_solid_(node->attr("solid") == "yes"),
    passthrough_(wml::get_bool(node, "passthrough")),
    flip_(wml::get_bool(node, "flip", false)),
    friction_(wml::get_int(node, "friction", 20)),
    traction_(wml::get_int(node, "traction", 100)),
    damage_(wml::get_int(node, "damage", 0))
{
	std::vector<std::string> tile_variations = util::split(node->attr("tiles"), '|');
	foreach(const std::string& variation, tile_variations) {
		tiles_.resize(tiles_.size()+1);
		std::vector<std::string> lines = util::split(variation, '\n');
		foreach(const std::string& line, lines) {
			std::vector<std::string> items = util::split(line);
			if(width_ == -1) {
				width_ = items.size();
			}

			items.resize(width_);

			foreach(const std::string& item, items) {
				try {
					const int width = std::max<int>(t_.width(), t_.height());
					assert(width%16 == 0);
					const int base = width/16;
					tiles_.back().push_back(strtol(item.c_str(), NULL, base));
				} catch(boost::bad_lexical_cast&) {
					tiles_.back().push_back(-1);
				}
			}
		}
	}

	if(tiles_.empty()) {
		tiles_.resize(1);
	}

	if(node->has_attr("solid_map")) {
		solid_.resize(width()*height());
		graphics::surface surf(graphics::surface_cache::get(node->attr("solid_map")).convert_opengl_format());
		if(surf.get()) {
			const uint32_t* p = reinterpret_cast<const uint32_t*>(surf->pixels);
			for(int n = 0; n != surf->w*surf->h && n != solid_.size(); ++n) {
				uint8_t r, g, b, alpha;
				SDL_GetRGBA(p[n], surf->format, &r, &g, &b, &alpha);
				if(alpha > 64) {
					solid_[n] = true;
				}
			}
		}
	}

	if(all_solid_) {
		solid_ = std::vector<bool>(width()*height(), true);
	} else if(node->attr("solid") == "diagonal") {
		solid_.resize(width()*height());
		for(int x = 0; x < width(); ++x) {
			for(int y = 0; y < height(); ++y) {
				const int index = y*width() + x;
				solid_[index] = (y >= x);
			}
		}
	} else if(node->attr("solid") == "reverse_diagonal") {
		solid_.resize(width()*height());
		for(int x = 0; x < width(); ++x) {
			for(int y = 0; y < height(); ++y) {
				const int index = y*width() + x;
				solid_[index] = (y >= (width() - x));
			}
		}
	} else if(node->has_attr("solid_heights")) {
		//this is a csv list of heights which represent the solids
		std::vector<std::string> heights = util::split(node->attr("solid_heights"));
		if(!heights.empty()) {
			solid_.resize(width()*height());
			for(int x = 0; x < width(); ++x) {
				const int heights_index = (heights.size()*x)/width();
				assert(heights_index >= 0 && heights_index < heights.size());
				const std::string& height_str = heights[heights_index];
				const int h = atoi(height_str.c_str());
				for(int y = height() - h; y < height(); ++y) {
					const int index = y*width() + x;
					solid_[index] = true;
				}
			}
		}
	}

	wml::node::const_child_iterator r1 = node->begin_child("rect");
	wml::node::const_child_iterator r2 = node->end_child("rect");
	for(; r1 != r2; ++r1) {
		const int x = wml::get_int(r1->second, "x");
		const int y = wml::get_int(r1->second, "y");
		const int w = wml::get_int(r1->second, "w");
		const int h = wml::get_int(r1->second, "h");

		if(solid_.empty()) {
			solid_.resize(width()*height());
		}
		for(int xpos = x; xpos != x+w; ++xpos) {
			for(int ypos = y; ypos != y+h; ++ypos) {
				if(xpos >= 0 && xpos < width() && ypos >= 0 && ypos < height()) {
					const int index = ypos*width() + xpos;
					assert(index >= 0 && index < solid_.size());
					solid_[index] = true;
				}
			}
		}
	}
}

int level_object::width() const
{
	return width_*32;
}

int level_object::height() const
{
	return (tiles_.front().size()/width_)*32;
}

bool level_object::is_solid(int x, int y) const
{
	if(solid_.empty()) {
		return false;
	}

	if(x < 0 || y < 0 || x >= width() || y >= height()) {
		return false;
	}

	const int index = y*width() + x;
	assert(index >= 0 && index < solid_.size());
	return solid_[index];
}

namespace {
int hash_level_object(int x, int y) {
	x = (x + 92872873) ^ 918273;
	y = (y + 1672517) ^ 128123;
	return x*y + x + y;
}
}

void level_object::draw(const level_tile& t)
{
	int index = 0;
	const std::vector<int>& tiles = t.object->tiles_[hash_level_object(t.x,t.y)%t.object->tiles_.size()];
	foreach(int i, tiles) {
		if(i < 0) {
			continue;
		}

		int x = index%t.object->width_;
		if(t.face_right) {
			x = t.object->width_ - x - 1;
		}
		const int y = index/t.object->width_;
		draw_from_tilesheet(t.object->t_, i, t.x + x*32, t.y + y*32, t.face_right, t.rotate);
		++index;
	}
}
