#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>

#include "asserts.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "hex_map.hpp"
#include "hex_object.hpp"
#include "hex_tile.hpp"
#include "json_parser.hpp"
#include "module.hpp"
#include "variant_utils.hpp"

namespace hex {

static const int HexTileSize = 72;

hex_map::hex_map(variant node)
	: zorder_(node["zorder"].as_int(-1000)), 
	x_(node["x"].as_int(0)), 
	y_(node["y"].as_int(0)), width_(0), height_(0)
{
	std::vector<std::string> result;
	std::string tile_str = node["tiles"].as_string();
	boost::algorithm::erase_all(tile_str, "\t");
	boost::algorithm::erase_all(tile_str, "\v");
	boost::algorithm::erase_all(tile_str, " ");
	boost::algorithm::erase_all(tile_str, "\r");
	boost::algorithm::split(result, tile_str, std::bind2nd(std::equal_to<char>(), '\n'));
	int x = x_;
	int y = y_;
	foreach(const std::string& row, result) {
		if(row.empty()) {
			y++;
			x = x_;
			continue;
		}
		std::vector<std::string> row_res;
		boost::algorithm::split(row_res, row, std::bind2nd(std::equal_to<char>(), ','));
		std::vector<hex_object_ptr> new_row;
		foreach(const std::string& col, row_res) {
			new_row.push_back(hex_object_ptr(new hex_object(col, x, y, this)));
			x++;
		}
		y++;
		x = x_;
		tiles_.push_back(new_row);
		width_ = std::max<size_t>(width_, tiles_.back().size());		
	}
	height_ = tiles_.size();
}

void hex_map::draw() const
{
	foreach(const hex_tile_row& row, tiles_) {
		foreach(const hex_object_ptr& col, row) {
			col->draw();
		}
	}
}

void hex_map::build()
{
	foreach(const std::string& rule, hex_object::get_rules()) {
		foreach(hex_tile_row& row, tiles_) {
			foreach(hex_object_ptr& col, row) {
				col->apply_rules(rule);
			}
		}
	}
}

std::string hex_map::make_tile_string() const
{
	std::ostringstream tiles;
	foreach(const hex_tile_row& row, tiles_) {
		for(hex_tile_row::const_iterator i = row.begin(); i != row.end(); ++i) {
			tiles << (*i)->type() << ((i+1) == row.end() ? "\n" : ",");
		}
	}
	return tiles.str();
}

variant hex_map::write() const
{
	variant_builder res;
	res.add("x", x_);
	res.add("y", y_);
	res.add("zorder", zorder_);

	res.add("tiles", make_tile_string());
	return res.build();
}

hex_object_ptr hex_map::get_hex_tile(direction d, int x, int y) const
{
	int ox = x;
	int oy = y;
	x -= x_;
	y -= y_;
	if(d == NORTH) {
		y -= 1;
	} else if(d == SOUTH) {
		y += 1;
	} else if(d == NORTH_WEST) {
		y -= (abs(ox)%2==0) ? 1 : 0;
		x -= 1;
	} else if(d == NORTH_EAST) {
		y -= (abs(ox)%2==0) ? 1 : 0;
		x += 1;
	} else if(d == SOUTH_WEST) {
		y += (abs(ox)%2) ? 1 : 0;
		x -= 1;
	} else if(d == SOUTH_EAST) {
		y += (abs(ox)%2) ? 1 : 0;
		x += 1;
	} else {
		ASSERT_LOG(false, "Unrecognised direction: " << d);
	}
	if(x < 0 || y < 0 || size_t(y) >= tiles_.size()) {
		return hex_object_ptr();
	}
	if(size_t(x) >= tiles_[y].size()) {
		return hex_object_ptr();
	}
	return tiles_[y][x];
}

variant hex_map::get_value(const std::string& key) const
{
	if(key == "x_size") {
		return variant(width());
	} else if(key == "y_size") {
		return variant(height());
	} else if(key == "size") {
		std::vector<variant> v;
		v.push_back(variant(width()));
		v.push_back(variant(height()));
		return variant(&v);
	} else if(key == "map") {
		std::vector<variant> list;
		foreach(const hex_tile_row& row, tiles_) {
			std::vector<variant> rrow;
			for(hex_tile_row::const_iterator i = row.begin(); i != row.end(); ++i) {
				rrow.push_back(variant((*i).get()));
			}
			list.push_back(variant(&rrow));
		}
		return variant(&list);
	} else if(key == "mapstring") {
		return variant(make_tile_string());
	} else if(key == "maplist") {
		std::vector<variant> list;
		foreach(const hex_tile_row& row, tiles_) {
			std::vector<variant> rrow;
			for(hex_tile_row::const_iterator i = row.begin(); i != row.end(); ++i) {
				rrow.push_back(variant((*i)->type()));
			}
			list.push_back(variant(&rrow));
		}
		return variant(&list);
	}
	return variant();
}

void hex_map::set_value(const std::string& key, const variant& value)
{
}

point hex_map::get_tile_pos_from_pixel_pos(int mx, int my)
{
	const int tesselation_x_size = (3*HexTileSize)/2;
	const int tesselation_y_size = HexTileSize;
	const int x_base = (mx>=0) ? mx / tesselation_x_size * 2 : mx / tesselation_x_size * 2 - 2;
	const int x_mod  = (mx>=0) ? mx % tesselation_x_size : tesselation_x_size + (mx % tesselation_x_size);
	const int y_base = (my>=0) ? my / tesselation_y_size : my / tesselation_y_size - 1;
	const int y_mod  = (my>=0) ? my % tesselation_y_size : tesselation_y_size + (my % tesselation_y_size);
	const int m = 2;

	int x_modifier = 0;
	int y_modifier = 0;

	if(y_mod < tesselation_y_size / 2) {
		if((x_mod * m + y_mod) < (HexTileSize / 2)) {
			x_modifier = -1;
			y_modifier = -1;
		} else if ((x_mod * m - y_mod) < (HexTileSize * 3 / 2)) {
			x_modifier = 0;
			y_modifier = 0;
		} else {
			x_modifier = 1;
			y_modifier = -1;
		}

	} else {
		if((x_mod * m - (y_mod - HexTileSize / 2)) < 0) {
			x_modifier = -1;
			y_modifier = 0;
		} else if((x_mod * m + (y_mod - HexTileSize / 2)) < HexTileSize * 2) {
			x_modifier = 0;
			y_modifier = 0;
		} else {
			x_modifier = 1;
			y_modifier = 0;
		}
	}
	return point(x_base + x_modifier, y_base + y_modifier);
}

hex_object_ptr hex_map::get_tile_from_pixel_pos(int mx, int my) const
{
	point p = get_tile_pos_from_pixel_pos(mx, my);
	return get_tile_at(p.x, p.y);
}

point hex_map::get_pixel_pos_from_tile_pos(int x, int y)
{
	const int HexTileSizeHalf = HexTileSize/2;
	const int HexTileSizeThreeQuarters = (HexTileSize*3)/4;
	const int tx = x*HexTileSizeThreeQuarters;
	const int ty = HexTileSize*y + (abs(x)%2)*HexTileSizeHalf;
	return point(tx, ty);
}

hex_object_ptr hex_map::get_tile_at(int x, int y) const
{
	x -= x_;
	y -= y_;
	if(x < 0 || y < 0 || size_t(y) >= tiles_.size()) {
		return hex_object_ptr();
	}
	if(size_t(x) >= tiles_[y].size()) {
		return hex_object_ptr();
	}
	return tiles_[y][x];
}

bool hex_map::set_tile(int xx, int yy, const std::string& tile)
{
	point p = get_tile_pos_from_pixel_pos(xx, yy);

	// New tile position is outside current bounds, so enlarge.
	if(p.y < y()) {
		int needed_rows = y() - p.y;
		y_ = p.y;
		std::vector<hex_object_ptr> r;
		tiles_.insert(tiles_.begin(), r);
	}
	if(p.x < x()) {
		int needed_cols = x() - p.x;
		int n = x_;
		x_ = p.x;
		for(int j = 0; j < tiles_.size(); ++j) {
			for(int i = 0; i < needed_cols; ++i) {
				tiles_[j].insert(tiles_[j].begin(), new hex_object("Xv", n + x(), j + y(), this));
				--n;
			}
		}
	}

	const int tx = p.x - x();
	const int ty = p.y - y();
	bool changed = false;

	int needed_rows = int(size_t(ty)+1 - tiles_.size());
	changed |= (needed_rows > 0 );
	while(needed_rows-- > 0) {
		std::vector<hex_object_ptr> r;
		tiles_.push_back(r);
	}
	int needed_cols = int(size_t(tx)+1 - tiles_[ty].size());
	changed |= (needed_cols > 0 );
	int n = tx;
	while(needed_cols-- > 0) {
		// Add Void 
		tiles_[ty].push_back(hex_object_ptr(new hex_object("Xv", n + x(), ty + y(), this)));
		++n;
	}
	if(tiles_[ty][tx] == NULL || tiles_[ty][tx]->type() != tile) {
		tiles_[ty][tx].reset(new hex_object(tile, tx + x(), ty + y(), this));
		changed = true;
	}
	return changed;
}

point hex_map::loc_in_dir(int x, int y, direction d)
{
	int ox = x;
	int oy = y;
	if(d == NORTH) {
		y -= 1;
	} else if(d == SOUTH) {
		y += 1;
	} else if(d == NORTH_WEST) {
		y -= (abs(ox)%2==0) ? 1 : 0;
		x -= 1;
	} else if(d == NORTH_EAST) {
		y -= (abs(ox)%2==0) ? 1 : 0;
		x += 1;
	} else if(d == SOUTH_WEST) {
		y += (abs(ox)%2) ? 1 : 0;
		x -= 1;
	} else if(d == SOUTH_EAST) {
		y += (abs(ox)%2) ? 1 : 0;
		x += 1;
	} else {
		ASSERT_LOG(false, "Unrecognised direction: " << d);
	}
	return point(x, y);
}

point hex_map::loc_in_dir(int x, int y, const std::string& s)
{
	if(s == "north" || s == "n") {
		return loc_in_dir(x, y, NORTH);
	} else if(s == "south" || s == "s") {
		return loc_in_dir(x, y, SOUTH);
	} else if(s == "north_west" || s == "nw" || s == "northwest") {
		return loc_in_dir(x, y, NORTH_WEST);
	} else if(s == "north_east" || s == "ne" || s == "northeast") {
		return loc_in_dir(x, y, NORTH_EAST);
	} else if(s == "south_west" || s == "sw" || s == "southwest") {
		return loc_in_dir(x, y, SOUTH_WEST);
	} else if(s == "south_east" || s == "se" || s == "southeast") {
		return loc_in_dir(x, y, SOUTH_EAST);
	}
	ASSERT_LOG(false, "Unreognised direction " << s);
	return point();
}

}
