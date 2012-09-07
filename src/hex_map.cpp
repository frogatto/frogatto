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

hex_map::hex_map(variant node)
	: zorder_(node["zorder"].as_int(-1000)), 
	x_(node["x"].as_int(0)), 
	y_(node["y"].as_int(0))
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

variant hex_map::write() const
{
	variant_builder res;
	res.add("x", x_);
	res.add("y", y_);
	res.add("zorder", zorder_);

	std::ostringstream tiles;
	foreach(const hex_tile_row& row, tiles_) {
		for(hex_tile_row::const_iterator i = row.begin(); i != row.end(); ++i) {
			tiles << (*i)->type() << ((i+1) == row.end() ? "\n" : ",");
		}
	}
	res.add("tiles", tiles.str());
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

}