#pragma once
#ifndef HEX_OBJECT_HPP_INCLUDED
#define HEX_OBJECT_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <vector>

#include "graphics.hpp"
#include "formula_callable.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "variant.hpp"

namespace hex {

class hex_map;

class hex_tile : public game_logic::formula_callable
{
public:
	explicit hex_tile(const std::string& key, variant node);
	virtual ~hex_tile();
	virtual void draw(int x, int y) const;
	std::string key() const { return key_; }
protected:
	virtual variant get_value(const std::string&) const;
	virtual void set_value(const std::string& key, const variant& value);
private:
	std::string key_;
	std::string name_;
	graphics::texture texture_;
	std::vector<rect> rects_;

	// Private default constructor and copy constructor to stop them
	// from being used.
	hex_tile() {}
	hex_tile(hex_tile&) {}
};

typedef boost::intrusive_ptr<hex_tile> hex_tile_ptr;
typedef boost::intrusive_ptr<const hex_tile> const_hex_tile_ptr;

class hex_object : public game_logic::formula_callable
{
public:
	hex_object(const std::string& type, int x, int y, const hex_map* owner);
	virtual variant get_value(const std::string&) const;
	virtual void set_value(const std::string& key, const variant& value);
	virtual void draw() const;
	
	void build();
	void apply_rules();
private:
	// map coordinates.
	int x_;
	int y_;
	// Pointer to the tile in this square.
	hex_tile_ptr tile_;
	// String representing the base type of this tile.
	std::string type_;
	// raw pointer to the map that owns this.
	const hex_map* owner_map_;

	// Private default constructor and copy constructor to stop them
	// from being used.
	hex_object() {}
	hex_object(hex_object&) {}
};

typedef boost::intrusive_ptr<hex_object> hex_object_ptr;
typedef boost::intrusive_ptr<const hex_object> const_hex_object_ptr;

}

#endif
