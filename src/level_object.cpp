#include <iostream>
#include <map>
#include <string>

#include "IMG_savepng.h"
#include "asserts.hpp"
#include "draw_tile.hpp"
#include "filesystem.hpp"
#include "level_object.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "string_utils.hpp"
#include "surface.hpp"
#include "surface_cache.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"
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
	return res;
}

namespace {
std::vector<wml::const_node_ptr> level_object_index;

//a tile identifier made up of a filename and tile position.
typedef std::pair<std::string, int> tile_id;
std::map<tile_id, int> compiled_tile_ids;
}

//defined in texture.cpp
namespace graphics {
void set_alpha_for_transparent_colors_in_rgba_surface(SDL_Surface* s);
}

void create_compiled_tiles_image()
{
	graphics::surface s(SDL_CreateRGBSurface(SDL_SWSURFACE, 1024, (compiled_tile_ids.size()/64 + 1)*16, 32, SURFACE_MASK));
	for(std::map<tile_id, int>::const_iterator itor = compiled_tile_ids.begin();
	    itor != compiled_tile_ids.end(); ++itor) {
		graphics::surface src = graphics::surface_cache::get(itor->first.first);
		SDL_SetAlpha(src.get(), 0, SDL_ALPHA_OPAQUE);
		const int width = std::max<int>(src->w, src->h)/16;

		const int src_x = (itor->first.second%width) * 16;
		const int src_y = (itor->first.second/width) * 16;
		
		const int dst_x = (itor->second%64) * 16;
		const int dst_y = (itor->second/64) * 16;

		SDL_Rect src_rect = { src_x, src_y, 16, 16 };
		SDL_Rect dst_rect = { dst_x, dst_y, 16, 16 };

		std::cerr << "BLIT COMPILED TILES: " << itor->first.first << ", " << itor->first.second << " -> " << itor->second << ": " << dst_x << ", " << dst_y << "\n";

		SDL_BlitSurface(src.get(), &src_rect, s.get(), &dst_rect);
	}

	graphics::set_alpha_for_transparent_colors_in_rgba_surface(s.get());

	IMG_SavePNG("images/tiles-compiled.png", s.get(), 5);
}

level_object::level_object(wml::const_node_ptr node)
  : id_(node->attr("id")), t_(graphics::texture::get(node->attr("image"))),
	all_solid_(node->attr("solid").str() == "yes"),
    passthrough_(wml::get_bool(node, "passthrough")),
    flip_(wml::get_bool(node, "flip", false)),
    friction_(wml::get_int(node, "friction", 100)),
    traction_(wml::get_int(node, "traction", 100)),
    damage_(wml::get_int(node, "damage", 0)),
	opaque_(wml::get_bool(node, "opaque", false)),
	draw_area_(0, 0, 16, 16),
	tile_index_(-1)
{
	if(node->has_attr("solid_color")) {
		solid_color_ = boost::intrusive_ptr<graphics::color>(new graphics::color(node->attr("solid_color")));
	}

	if(node->has_attr("draw_area")) {
		draw_area_ = rect(node->attr("draw_area"));
	}

	std::vector<std::string> tile_variations = util::split(node->attr("tiles"), '|');
	foreach(const std::string& variation, tile_variations) {
		if(!variation.empty() && variation[0] == '+') {
			//a + symbol at the start of tiles means that it's just a base-10
			//number. This is generally what is used for compiled tiles.
			tiles_.push_back(strtol(variation.c_str()+1, NULL, 10));
		} else {
			const int width = std::max<int>(t_.width(), t_.height());
			assert(width%16 == 0);
			const int base = std::min<int>(32, width/16);
			tiles_.push_back(strtol(variation.c_str(), NULL, base));
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
	
	std::vector<std::string> solid_attr = util::split(node->attr("solid").str());

	if(all_solid_ || std::find(solid_attr.begin(), solid_attr.end(), "flat") != solid_attr.end()) {
		if(passthrough_){
			solid_.resize(width()*height());
			for(int x = 0; x < width(); ++x) {
				for(int y = 0; y < height(); ++y) {
					const int index = y*width() + x;
					solid_[index] = (y == 0);
				}
			}
			//set all_solid_ to false because it's not longer the case.
			all_solid_ = false;
		}else{
			solid_ = std::vector<bool>(width()*height(), true);
		}
	}
		
	if(std::find(solid_attr.begin(), solid_attr.end(), "diagonal") != solid_attr.end()) {
		solid_.resize(width()*height());
		for(int x = 0; x < width(); ++x) {
			for(int y = 0; y < height(); ++y) {
				const int index = y*width() + x;
				solid_[index] = solid_[index] || (passthrough_? (y==x) : (y >= x));
			}
		}
	}
	
	if(std::find(solid_attr.begin(), solid_attr.end(), "reverse_diagonal") != solid_attr.end()) {
		solid_.resize(width()*height());
		for(int x = 0; x < width(); ++x) {
			for(int y = 0; y < height(); ++y) {
				const int index = y*width() + x;
				solid_[index] = solid_[index] || (passthrough_? (y == (width() - (x+1))) : (y >= (width() - (x+1))));
			}
		}
	}
	
	if(std::find(solid_attr.begin(), solid_attr.end(), "quarter_diagonal_lower") != solid_attr.end()) {
		solid_.resize(width()*height());
		for(int x = 0; x < width(); ++x) {
			for(int y = 0; y < height(); ++y) {
				const int index = y*width() + x;
				solid_[index] = solid_[index] || (passthrough_? (y == (x/2 + width()/2)) : (y >= (x/2 + width()/2)));
			}
		}
	}
	
	if(std::find(solid_attr.begin(), solid_attr.end(), "quarter_diagonal_upper") != solid_attr.end()) {
		solid_.resize(width()*height());
		for(int x = 0; x < width(); ++x) {
			for(int y = 0; y < height(); ++y) {
				const int index = y*width() + x;
				solid_[index] = solid_[index] || (passthrough_? (y == x/2) : (y >= x/2));
			}
		}
	}
	
	if(std::find(solid_attr.begin(), solid_attr.end(), "reverse_quarter_diagonal_lower") != solid_attr.end()) {
		solid_.resize(width()*height());
		for(int x = 0; x < width(); ++x) {
			for(int y = 0; y < height(); ++y) {
				const int index = y*width() + x;
				solid_[index] = solid_[index] || (passthrough_? (y == (width() - x/2) -1) : (y >= (width() - x/2)));
			}
		}
	}
	
	if(std::find(solid_attr.begin(), solid_attr.end(), "reverse_quarter_diagonal_upper") != solid_attr.end()) {
		solid_.resize(width()*height());
		for(int x = 0; x < width(); ++x) {
			for(int y = 0; y < height(); ++y) {
				const int index = y*width() + x;
				solid_[index] = solid_[index] || (passthrough_? (y == (width()/2 - x/2) -1) : (y >= (width()/2 - x/2)));
			}
		}
	}
	
	if(node->has_attr("solid_heights")) {
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
	
	if(preferences::compiling_tiles) {
		tile_index_ = level_object_index.size();
		wml::node_ptr node_copy(wml::deep_copy(node));
		if(calculate_opaque()) {
			node_copy->set_attr("opaque", "yes");
			opaque_ = true;
		}

		graphics::color col;
		if(calculate_is_solid_color(col)) {
			node_copy->set_attr("solid_color", graphics::color_transform(col).to_string());
		}

		if(calculate_draw_area()) {
			node_copy->set_attr("draw_area", draw_area_.to_string());
		}

		std::string tiles_str;

		foreach(int tile, tiles_) {
			tile_id id(node_copy->attr("image"), tile);
			std::map<tile_id, int>::const_iterator itor = compiled_tile_ids.find(id);
			if(itor == compiled_tile_ids.end()) {
				compiled_tile_ids[id] = compiled_tile_ids.size();
				itor = compiled_tile_ids.find(id);
			}

			const int index = itor->second;

			char tile_pos[64];
			sprintf(tile_pos, "+%d", index);
			if(!tiles_str.empty()) {
				tiles_str += "|";
			}

			tiles_str += tile_pos;
		}

		node_copy->set_attr("image", "tiles-compiled.png");
		node_copy->set_attr("tiles", tiles_str);

		level_object_index.push_back(node_copy);

		//set solid colors to always false if we're compiling, since having
		//solid colors will confuse the compilation.
		solid_color_ = boost::intrusive_ptr<graphics::color>();
	}

	//debug code to output the solidity of a tile in case we need to introspect at some point
	/*
	std::cerr << "LEVEL_OBJECT: " << wml::output(node) << ":::\nSOLID:::\n";
	if(solid_.size() == height()*width()) {
		for(int y = 0; y != height(); ++y) {
			for(int x = 0; x != width(); ++x) {
				std::cerr << (solid_[y*width() + x] ? "1" : "0");
			}
			
			std::cerr << "\n";
		}
	} else {
		std::cerr << "SOLID SIZE: " << solid_.size() << "\n";
	}
	*/
}

namespace {
const char base64_chars[65] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.!";

std::vector<int> create_base64_char_to_int()
{
	std::vector<int> result(256);
	int index = 0;
	foreach(char c, base64_chars) {
		result[c] = index++;
	}

	return result;
}

std::vector<int> base64_char_to_int = create_base64_char_to_int();

void base64_encode(int num, char* buf, int nplaces)
{
	buf[nplaces--] = 0;
	while(num > 0 && nplaces >= 0) {
		buf[nplaces--] = base64_chars[num%64];
		num /= 64;
	}

	while(nplaces >= 0) {
		buf[nplaces--] = '0';
	}
}

int base64_unencode(const char* begin, const char* end)
{
	int result = 0;
	while(begin != end) {
		result = result*64 + base64_char_to_int[*begin];
		++begin;
	}

	return result;
}

}

void level_object::write_compiled()
{
	for(int n = 0; n <= level_object_index.size()/64; ++n) {
		char buf[128];
		sprintf(buf, "%d", n);
		const std::string filename = std::string(buf) + ".cfg";
		wml::node_ptr tiles_node(new wml::node("tiles"));
		for(int m = n*64; m < level_object_index.size() && m < (n+1)*64; ++m) {
			tiles_node->add_child(wml::deep_copy(level_object_index[m]));
		}

		std::string data;
		wml::write(tiles_node, data);
		sys::write_file("data/compiled/tiles/" + filename, data);
	}

	create_compiled_tiles_image();
}

namespace {
std::vector<const_level_object_ptr> compiled_tiles;

void load_compiled_tiles(int index)
{
	int starting_index = index*64;
	char buf[128];
	sprintf(buf, "%d", index);
	wml::const_node_ptr node(wml::parse_wml_from_file("data/compiled/tiles/" + std::string(buf) + ".cfg"));
	int count = 0;
	for(wml::node::const_all_child_iterator i = node->begin_children(); i != node->end_children(); ++i) {
		if(starting_index >= compiled_tiles.size()) {
			compiled_tiles.resize(starting_index+64);
		}
		compiled_tiles[starting_index++].reset(new level_object(*i));
		++count;
	}
}

}

const_level_object_ptr level_object::get_compiled(const char* buf)
{
	const int index = base64_unencode(buf, buf+3);
	if(index >= compiled_tiles.size() || !compiled_tiles[index]) {
		load_compiled_tiles(base64_unencode(buf, buf+2));
	}

	ASSERT_LOG(index >= compiled_tiles.size() || compiled_tiles[index], "COULD NOT LOAD COMPILED TILE: " << std::string(buf, buf+3) << " -> " << index);

	return compiled_tiles[index];
}

void level_object::write_compiled_index(char* buf) const
{
	base64_encode(tile_index_, buf, 3);
}

int level_object::width() const
{
	return 32;
}

int level_object::height() const
{
	return 32;
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
	x /= 32;
	y /= 32;
	x = (x + 92872873) ^ 918273;
	y = (y + 1672517) ^ 128123;
	return (x*y + x + y)*1103515245 + 12345;
}
}

void level_object::queue_draw(graphics::blit_queue& q, const level_tile& t)
{
	const int random_index = hash_level_object(t.x,t.y);
	const int tile = t.object->tiles_[random_index%t.object->tiles_.size()];

	queue_draw_from_tilesheet(q, t.object->t_, t.object->draw_area_, tile, t.x, t.y, t.face_right);
}

int level_object::calculate_tile_corners(tile_corner* result, const level_tile& t)
{
	const int random_index = hash_level_object(t.x,t.y);
	const int tile = t.object->tiles_[random_index%t.object->tiles_.size()];

	return get_tile_corners(result, t.object->t_, t.object->draw_area_, tile, t.x, t.y, t.face_right);
}

bool level_object::calculate_opaque() const
{
	foreach(int tile, tiles_) {
		if(!is_tile_opaque(t_, tile)) {
			return false;
		}
	}

	return true;
}

bool level_object::calculate_is_solid_color(graphics::color& col) const
{
	foreach(int tile, tiles_) {
		if(!is_tile_solid_color(t_, tile, col)) {
			return false;
		}
	}

	return true;
}

bool level_object::calculate_draw_area()
{
	draw_area_ = rect();
	foreach(int tile, tiles_) {
		draw_area_ = rect_union(draw_area_, get_tile_non_alpha_area(t_, tile));
	}

	return draw_area_ != rect(0, 0, 16, 16);
}
