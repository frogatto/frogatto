#include <iostream>
#include <sstream>
#include <map>
#include <string>

#include "IMG_savepng.h"
#include "asserts.hpp"
#include "draw_tile.hpp"
#include "filesystem.hpp"
#include "json_parser.hpp"
#include "level_object.hpp"
#include "module.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "string_utils.hpp"
#include "surface.hpp"
#include "surface_cache.hpp"
#include "surface_palette.hpp"
#include "variant_utils.hpp"

namespace {
typedef std::map<std::string,const_level_object_ptr> tiles_map;
tiles_map tiles_cache;

const int BaseTileSize = 16;
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

level_tile level_object::build_tile(variant node)
{
	level_tile res;
	res.x = node["x"].as_int();
	res.y = node["y"].as_int();
	res.zorder = node["zorder"].as_int();
	if(tiles_cache.count(node["tile"].as_string())) {
		res.object = tiles_cache[node["tile"].as_string()].get();
	}
	res.face_right = node["face_right"].as_bool();
	return res;
}

namespace {

typedef boost::shared_ptr<variant> obj_variant_ptr;
typedef boost::shared_ptr<const variant> const_obj_variant_ptr;

std::vector<obj_variant_ptr> level_object_index;
std::vector<const_obj_variant_ptr> original_level_object_nodes;
std::map<std::pair<const_obj_variant_ptr, int>, level_object_ptr> secondary_zorder_objects;

std::map<obj_variant_ptr, int> tile_nodes_to_zorders;


typedef std::pair<std::string, int> filename_palette_pair;

//a tile identifier made up of a filename, palette and tile position.
typedef std::pair<filename_palette_pair, int> tile_id;
std::map<tile_id, int> compiled_tile_ids;
}

//defined in texture.cpp
namespace graphics {
void set_alpha_for_transparent_colors_in_rgba_surface(SDL_Surface* s, int options=0);
}

void create_compiled_tiles_image()
{
	//the number of tiles that can fit in a tiesheet.
	const int TilesInSheet = (1024*1024)/(BaseTileSize*BaseTileSize);

	//calculate how many tiles are in each zorder
	std::map<int, int> zorder_to_num_tiles;
	for(std::map<obj_variant_ptr, int>::const_iterator i = tile_nodes_to_zorders.begin(); i != tile_nodes_to_zorders.end(); ++i) {
		zorder_to_num_tiles[i->second]++;
	}

	//now work out which zorders should go in which tilesheets.
	//all tiles of the same zorder always go in the same sheet.
	std::map<int, int> zorder_to_sheet_number;
	std::vector<int> tiles_in_sheet;
	std::vector<graphics::surface> sheets;
	std::vector<int> sheet_next_image_index;

	for(std::map<int, int>::const_iterator i = zorder_to_num_tiles.begin();
	    i != zorder_to_num_tiles.end(); ++i) {
		int sheet = 0;
		for(; sheet != tiles_in_sheet.size(); ++sheet) {
			if(tiles_in_sheet[sheet] + i->second <= TilesInSheet) {
				break;
			}
		}

		if(sheet == tiles_in_sheet.size()) {
			tiles_in_sheet.push_back(0);
			sheet_next_image_index.push_back(0);
			sheets.push_back(graphics::surface(SDL_CreateRGBSurface(SDL_SWSURFACE, 1024, 1024, 32, SURFACE_MASK)));
		}

		tiles_in_sheet[sheet] += i->second;
		zorder_to_sheet_number[i->first] = sheet;
	}

	std::cerr << "NUM_TILES: " << tile_nodes_to_zorders.size() << " / " << TilesInSheet << "\n";


	graphics::surface s(SDL_CreateRGBSurface(SDL_SWSURFACE, 1024, (compiled_tile_ids.size()/64 + 1)*BaseTileSize, 32, SURFACE_MASK));
	for(std::map<tile_id, int>::const_iterator itor = compiled_tile_ids.begin();
	    itor != compiled_tile_ids.end(); ++itor) {
		const tile_id& tile_info = itor->first;
		const std::string& filename = tile_info.first.first;
		const int palette = tile_info.first.second;
		const int tile_pos = tile_info.second;

		std::cerr << "WRITING PALETTE: " << palette << "\n";

		graphics::surface src = graphics::surface_cache::get(filename);
		if(palette >= 0) {
			src = graphics::map_palette(src, palette);
		}

		SDL_SetAlpha(src.get(), 0, SDL_ALPHA_OPAQUE);
		const int width = std::max<int>(src->w, src->h)/BaseTileSize;

		const int src_x = (tile_pos%width) * BaseTileSize;
		const int src_y = (tile_pos/width) * BaseTileSize;
		
		const int dst_x = (itor->second%64) * BaseTileSize;
		const int dst_y = (itor->second/64) * BaseTileSize;

		SDL_Rect src_rect = { src_x, src_y, BaseTileSize, BaseTileSize };
		SDL_Rect dst_rect = { dst_x, dst_y, BaseTileSize, BaseTileSize };

		SDL_BlitSurface(src.get(), &src_rect, s.get(), &dst_rect);
	}

	graphics::set_alpha_for_transparent_colors_in_rgba_surface(s.get());

//  don't need to save the main compiled tile anymore.
//	IMG_SavePNG("images/tiles-compiled.png", s.get(), 5);

	SDL_SetAlpha(s.get(), 0, SDL_ALPHA_OPAQUE);

	for(std::map<obj_variant_ptr, int>::const_iterator i = tile_nodes_to_zorders.begin(); i != tile_nodes_to_zorders.end(); ++i) {
		const int sheet = zorder_to_sheet_number[i->second];
		obj_variant_ptr node = i->first;
		std::map<int, int> dst_index_map;

		std::vector<std::string> tiles_vec = util::split((*node)["tiles"].as_string(), '|');
		std::string tiles_val;

		foreach(const std::string& tiles_str, tiles_vec) {
			ASSERT_EQ(tiles_str[0], '+');

			const int tile_num = atoi(tiles_str.c_str() + 1);
			const int src_x = (tile_num%64) * BaseTileSize;
			const int src_y = (tile_num/64) * BaseTileSize;

			int dst_tile;
			if(dst_index_map.count(tile_num)) {
				dst_tile = dst_index_map[tile_num];
			} else {
				dst_tile = sheet_next_image_index[sheet]++;
				dst_index_map[tile_num] = dst_tile;

				const int dst_x = (dst_tile%64) * BaseTileSize;
				const int dst_y = (dst_tile/64) * BaseTileSize;

				SDL_Rect src_rect = { src_x, src_y, BaseTileSize, BaseTileSize };
				SDL_Rect dst_rect = { dst_x, dst_y, BaseTileSize, BaseTileSize };

				SDL_BlitSurface(s.get(), &src_rect, sheets[sheet].get(), &dst_rect);
			}

			if(!tiles_val.empty()) {
				tiles_val += "|";
			}

			char buf[128];
			sprintf(buf, "+%d", dst_tile);
			tiles_val += buf;
		}

		*node = node->add_attr(variant("tiles"), variant(tiles_val));

		char buf[128];
		sprintf(buf, "tiles-compiled-%d.png", sheet);

		*node = node->add_attr(variant("image"), variant(buf));
	}

	for(int n = 0; n != sheets.size(); ++n) {
		char buf[64];
		sprintf(buf, "images/tiles-compiled-%d.png", n);

		IMG_SavePNG(buf, sheets[n], 5);
	}
}

namespace {
unsigned int current_palette_set = 0;
std::set<level_object*>& palette_level_objects() {
	//we never want this to be destroyed, since it's too hard to
	//guarantee destruction order.
	static std::set<level_object*>* instance = new std::set<level_object*>;
	return *instance;
}
}

palette_scope::palette_scope(const std::vector<std::string>& v)
  : original_value(current_palette_set)
{
	current_palette_set = 0;	
	foreach(const std::string& pal, v) {
		const int id = graphics::get_palette_id(pal);
		current_palette_set |= 1 << id;
	}
}

palette_scope::~palette_scope()
{
	current_palette_set = original_value;
}

void level_object::set_current_palette(unsigned int palette)
{
	foreach(level_object* obj, palette_level_objects()) {
		obj->set_palette(palette);
	}
}

level_object::level_object(variant node)
  : id_(node["id"].as_string_default()), image_(node["image"].as_string()),
    info_(node["info"].as_string_default()),
    t_(graphics::texture::get(image_)),
	all_solid_(node["solid"].is_bool() ? node["solid"].as_bool() : node["solid"].as_string_default() == "yes"),
    passthrough_(node["passthrough"].as_bool()),
    flip_(node["flip"].as_bool(false)),
    friction_(node["friction"].as_int(100)),
    traction_(node["traction"].as_int(100)),
    damage_(node["damage"].as_int(0)),
	opaque_(node["opaque"].as_bool(false)),
	draw_area_(0, 0, BaseTileSize, BaseTileSize),
	tile_index_(-1),
	palettes_recognized_(current_palette_set),
	current_palettes_(0)
{
	if(node.has_key("palettes")) {
		palettes_recognized_ = 0;
		std::vector<std::string> p = parse_variant_list_or_csv_string(node["palettes"]);
		foreach(const std::string& pal, p) {
			const int id = graphics::get_palette_id(pal);
			palettes_recognized_ |= 1 << id;
		}
	}

	if(palettes_recognized_) {
		palette_level_objects().insert(this);
	}

	if(node.has_key("solid_color")) {
		solid_color_ = boost::intrusive_ptr<graphics::color>(new graphics::color(node["solid_color"].as_string_default()));
		if(preferences::use_16bpp_textures()) {
			*solid_color_ = graphics::color(graphics::map_color_to_16bpp(solid_color_->rgba()));
		}
	}

	if(node.has_key("draw_area")) {
		draw_area_ = rect(node["draw_area"].as_string_default());
	}

	//TODO: Fix up the JSON to be consistent and use a list.
	std::string tiles_str;
	variant tiles_variant = node["tiles"];
	if(tiles_variant.is_int()) {
		tiles_str = tiles_variant.string_cast();
	} else {
		tiles_str = tiles_variant.as_string();
	}

	std::vector<std::string> tile_variations = util::split(tiles_str, '|');
	foreach(const std::string& variation, tile_variations) {
		if(!variation.empty() && variation[0] == '+') {
			//a + symbol at the start of tiles means that it's just a base-10
			//number. This is generally what is used for compiled tiles.
			tiles_.push_back(strtol(variation.c_str()+1, NULL, 10));
		} else {
			const int width = std::max<int>(t_.width(), t_.height());
			assert(width%BaseTileSize == 0);
			const int base = std::min<int>(32, width/BaseTileSize);
			tiles_.push_back((base == 1) ? 0 : strtol(variation.c_str(), NULL, base));
		}
	}

	if(tiles_.empty()) {
		tiles_.resize(1);
	}

	if(node.has_key("solid_map")) {
		solid_.resize(width()*height());
		graphics::surface surf(graphics::surface_cache::get(node["solid_map"].as_string()).convert_opengl_format());
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
	
	std::vector<std::string> solid_attr;
	
	if(!node["solid"].is_bool()) {
		solid_attr = util::split(node["solid"].as_string_default());
	}

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
	
	if(std::find(solid_attr.begin(), solid_attr.end(), "upward_diagonal") != solid_attr.end()) {
		solid_.resize(width()*height());
		for(int x = 0; x < width(); ++x) {
			for(int y = 0; y < height(); ++y) {
				const int index = y*width() + x;
				solid_[index] = solid_[index] || (passthrough_? (y==x) : (y <= x));
			}
		}
	}

	if(std::find(solid_attr.begin(), solid_attr.end(), "upward_reverse_diagonal") != solid_attr.end()) {
		solid_.resize(width()*height());
		for(int x = 0; x < width(); ++x) {
			for(int y = 0; y < height(); ++y) {
				const int index = y*width() + x;
				solid_[index] = solid_[index] || (passthrough_? (y == (width() - (x+1))) : (y <= (width() - (x+1))));
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
	
	if(node.has_key("solid_heights")) {
		//this is a list of heights which represent the solids
		std::vector<int> heights = node["solid_heights"].as_list_int();
		if(!heights.empty()) {
			solid_.resize(width()*height());
			for(int x = 0; x < width(); ++x) {
				const int heights_index = (heights.size()*x)/width();
				assert(heights_index >= 0 && heights_index < heights.size());
				const int h = heights[heights_index];
				for(int y = height() - h; y < height(); ++y) {
					const int index = y*width() + x;
					solid_[index] = true;
				}
			}
		}
	}

	foreach(variant r, node["rect"].as_list()) {
		const int x = r["x"].as_int();
		const int y = r["y"].as_int();
		const int w = r["w"].as_int();
		const int h = r["h"].as_int();

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

		//set solid colors to always false if we're compiling, since having
		//solid colors will confuse the compilation.
		solid_color_ = boost::intrusive_ptr<graphics::color>();

		std::vector<int> palettes;
		palettes.push_back(-1);
		get_palettes_used(palettes);

		foreach(int palette, palettes) {
			variant node_copy = node.add_attr(variant("palettes"), variant());
			if(calculate_opaque()) {
				node_copy = node_copy.add_attr(variant("opaque"), variant(true));
				opaque_ = true;
			}

			graphics::color col;
			if(calculate_is_solid_color(col)) {
				if(palette >= 0) {
					col = graphics::map_palette(col, palette);
				}
				node_copy = node_copy.add_attr(variant("solid_color"), variant(graphics::color_transform(col).to_string()));
			}

			if(calculate_draw_area()) {
				node_copy = node_copy.add_attr(variant("draw_area"), variant(draw_area_.to_string()));
			}

			std::string tiles_str;

			foreach(int tile, tiles_) {
				tile_id id(filename_palette_pair(node_copy["image"].as_string(), palette), tile);
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

			node_copy = node_copy.add_attr(variant("image"), variant("tiles-compiled.png"));
			node_copy = node_copy.add_attr(variant("tiles"), variant(tiles_str));
	
			level_object_index.push_back(obj_variant_ptr(new variant(node_copy)));
			original_level_object_nodes.push_back(const_obj_variant_ptr(new variant(node)));
		}
	}
}

level_object::~level_object()
{
	if(palettes_recognized_) {
		palette_level_objects().erase(this);
	}
}

namespace {
const char base64_chars[65] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.!";

std::vector<int> create_base64_char_to_int()
{
	std::vector<int> result(256);
	int index = 0;
	foreach(char c, (char const *)base64_chars) {
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
	create_compiled_tiles_image();

	for(int n = 0; n <= level_object_index.size()/64; ++n) {
		std::stringstream ss;
		ss << std::dec << n;
		const std::string filename = ss.str() + ".cfg";
		variant_builder tiles_node;
		for(int m = n*64; m < level_object_index.size() && m < (n+1)*64; ++m) {
			tiles_node.add("tiles", *level_object_index[m]);
		}

		module::write_file("data/compiled/tiles/" + filename, tiles_node.build().write_json(true));
	}
}

namespace {
std::vector<const_level_object_ptr> compiled_tiles;

void load_compiled_tiles(int index)
{
	int starting_index = index*64;
	char buf[128];
	sprintf(buf, "%d", index);
	variant node(json::parse_from_file("data/compiled/tiles/" + std::string(buf) + ".cfg"));
	int count = 0;

	foreach(variant tile_node, node["tiles"].as_list()) {
		if(starting_index >= compiled_tiles.size()) {
			compiled_tiles.resize(starting_index+64);
		}
		compiled_tiles[starting_index++].reset(new level_object(tile_node));
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

level_object_ptr level_object::record_zorder(int zorder) const
{
	std::vector<int>::const_iterator i = std::find(zorders_.begin(), zorders_.end(), zorder);
	if(i == zorders_.end()) {
		zorders_.push_back(zorder);
		if(zorders_.size() > 1) {
			level_object_ptr result(new level_object(*original_level_object_nodes[tile_index_]));
			result->zorders_.push_back(zorder);
			std::pair<const_obj_variant_ptr, int> key(original_level_object_nodes[tile_index_], zorder);

			secondary_zorder_objects[key] = result;

			std::vector<int> palettes;
			palettes.push_back(-1);
			result->get_palettes_used(palettes);

			for(int n = 0; n != palettes.size(); ++n) {
				tile_nodes_to_zorders[level_object_index[result->tile_index_ + n]] = zorder;
			}

			return result;
		} else {
			std::vector<int> palettes;
			palettes.push_back(-1);
			get_palettes_used(palettes);

			for(int n = 0; n != palettes.size(); ++n) {
				tile_nodes_to_zorders[level_object_index[tile_index_ + n]] = zorder;
			}
		}
	} else if(i != zorders_.begin()) {
		std::pair<const_obj_variant_ptr, int> key(original_level_object_nodes[tile_index_], zorder);
		return secondary_zorder_objects[key];
	}

	return level_object_ptr();
}

void level_object::write_compiled_index(char* buf) const
{
	if(current_palettes_ == 0) {
		base64_encode(tile_index_, buf, 3);
	} else {
		unsigned int mask = current_palettes_;
		int npalette = 0;
		while(!(mask&1)) {
			mask >>= 1;
			++npalette;
		}

		std::vector<int> palettes;
		get_palettes_used(palettes);
		std::vector<int>::const_iterator i = std::find(palettes.begin(), palettes.end(), npalette);
		ASSERT_LOG(i != palettes.end(), "PALETTE NOT FOUND: " << npalette);
		base64_encode(tile_index_ + 1 + (i - palettes.begin()), buf, 3);
	}
}

int level_object::width() const
{
	return BaseTileSize*2;
}

int level_object::height() const
{
	return BaseTileSize*2;
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

	static const unsigned int x_rng[] = {31,29,62,59,14,2,64,50,17,74,72,47,69,92,89,79,5,21,36,83,81,35,58,44,88,5,51,4,23,54,87,39,44,52,86,6,95,23,72,77,48,97,38,20,45,58,86,8,80,7,65,0,17,85,84,11,68,19,63,30,32,57,62,70,50,47,41,0,39,24,14,6,18,45,56,54,77,61,2,68,92,20,93,68,66,24,5,29,61,48,5,64,39,91,20,69,39,59,96,33,81,63,49,98,48,28,80,96,34,20,65,84,19,87,43,4,54,21,35,54,66,28,42,22,62,13,59,42,17,66,67,67,55,65,20,68,75,62,58,69,95,50,34,46,56,57,71,79,80,47,56,31,35,55,95,60,12,76,53,52,94,90,72,37,8,58,9,70,5,89,61,27,28,51,38,58,60,46,25,86,46,0,73,7,66,91,13,92,78,58,28,2,56,3,70,81,19,98,50,50,4,0,57,49,36,4,51,78,10,7,26,44,28,43,53,56,53,13,6,71,95,36,87,49,62,63,30,45,75,41,59,51,77,0,72,28,24,25,35,4,4,56,87,23,25,21,4,58,57,19,4,97,78,31,38,80,};
	static const unsigned int y_rng[] = {91,80,42,50,40,7,82,67,81,3,54,31,74,49,30,98,49,93,7,62,10,4,67,93,28,53,74,20,36,62,54,64,60,33,85,31,31,6,22,2,29,16,63,46,83,78,2,11,18,39,62,56,36,56,0,39,26,45,72,46,11,4,49,13,24,40,47,51,17,99,80,64,27,21,20,4,1,37,33,25,9,87,87,36,44,4,77,72,23,73,76,47,28,41,94,69,48,81,82,0,41,7,90,75,4,37,8,86,64,14,1,89,91,0,29,44,35,36,78,89,40,86,19,5,39,52,24,42,44,74,71,96,78,29,54,72,35,96,86,11,49,96,90,79,79,70,50,36,15,50,34,31,86,99,77,97,19,15,32,54,58,87,79,85,49,71,91,78,98,64,18,82,55,66,39,35,86,63,87,41,25,73,79,99,43,2,29,16,53,42,43,26,45,45,95,70,35,75,55,73,58,62,45,86,46,90,12,10,72,88,29,77,10,8,92,72,22,3,1,49,5,51,41,86,65,66,95,23,60,87,64,86,55,30,48,76,21,76,43,52,52,23,40,64,69,43,69,97,34,39,18,87,46,8,96,50,};

	return x_rng[x%(sizeof(x_rng)/sizeof(*x_rng))] +
	       y_rng[y%(sizeof(y_rng)/sizeof(*y_rng))];
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

	return draw_area_ != rect(0, 0, BaseTileSize, BaseTileSize);
}

void level_object::set_palette(unsigned int palette)
{
	palette &= palettes_recognized_;
	if(palette == current_palettes_) {
		return;
	}

	current_palettes_ = palette;

	if(palette == 0) {
		t_ = graphics::texture::get(image_);
	} else {
		int npalette = 0;
		while((palette&1) == 0) {
			palette >>= 1;
			++npalette;
		}

		t_ = graphics::texture::get_palette_mapped(image_, npalette);
	}
}

void level_object::get_palettes_used(std::vector<int>& v) const
{
	unsigned int p = palettes_recognized_;
	int palette = 0;
	while(p) {
		if(p&1) {
			v.push_back(palette);
		}

		p >>= 1;
		++palette;
	}
}

variant level_object::get_value(const std::string& key) const
{
	if(key == "id") {
		return variant(id_);
	} else if(key == "info") {
		return variant(info_);
	} else if(key == "damage") {
		return variant(damage_);
	} else if(key == "friction") {
		return variant(friction_);
	} else if(key == "traction") {
		return variant(traction_);
	} else {
		return variant();
	}
}
