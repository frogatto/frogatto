#pragma once
#ifndef HEX_MAP_HPP_INCLUDED
#define HEX_MAP_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <vector>

namespace hex {
	enum direction {NORTH, SOUTH, NORTH_WEST, NORTH_EAST, SOUTH_WEST, SOUTH_EAST};
}

#include "hex_object_fwd.hpp"
#include "hex_object.hpp"
#include "formula_callable.hpp"
#include "variant.hpp"

namespace hex {

typedef std::vector<hex_object_ptr> hex_tile_row;
typedef std::vector<hex_tile_row> hex_tile_map;

class hex_map : public game_logic::formula_callable
{
public:
	hex_map() : zorder_(-1000), width_(0), height_(0), x_(0), y_(0)
	{}
	virtual ~hex_map()
	{}
	explicit hex_map(variant node);
	int zorder() const { return zorder_; }
	void set_zorder(int zorder) { zorder_ = zorder; }

	int x() const { return x_; }
	int y() const { return y_; }

	size_t width() const { return width_; }
	size_t height() const { return height_; }
	size_t size() const { return width_ * height_; }
	void build();
	virtual void draw() const;
	variant write() const;

	bool set_tile(int x, int y, const std::string& tile);

	hex_object_ptr get_hex_tile(direction d, int x, int y) const;
	hex_object_ptr get_tile_at(int x, int y) const;
	hex_object_ptr get_tile_from_pixel_pos(int x, int y) const;
	static point get_tile_pos_from_pixel_pos(int x, int y);
	static point get_pixel_pos_from_tile_pos(int x, int y);

	static point loc_in_dir(int x, int y, direction d);
	static point loc_in_dir(int x, int y, const std::string& s);
protected:
	virtual variant get_value(const std::string&) const;
	virtual void set_value(const std::string& key, const variant& value);

	std::string make_tile_string() const;
private:
	hex_tile_map tiles_;
	size_t width_;
	size_t height_;
	int x_;
	int y_;
	int zorder_;
};

typedef boost::intrusive_ptr<hex_map> hex_map_ptr;
typedef boost::intrusive_ptr<const hex_map> const_hex_map_ptr;

}

#endif
