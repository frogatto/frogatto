#include <boost/algorithm/string/split.hpp>

#include "foreach.hpp"
#include "hex_map.hpp"

namespace hex {

hex_map::hex_map(variant node)
	: zorder_(node["zorder"].as_int(-1000)), 
	x_(node["x"].as_int(0)), 
	y_(node["y"].as_int(0))
{
	std::vector<std::string> result;
	boost::algorithm::split(result, node["tiles"].as_string(), std::bind2nd(std::equal_to<char>(), '\n'));
	int x = x_;
	int y = y_;
	foreach(const std::string& row, result) {
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
	// Then apply the rules generated from FFL.
	foreach(hex_tile_row& row, tiles_) {
		foreach(hex_object_ptr& col, row) {
			col->apply_rules();
		}
	}
}

}
