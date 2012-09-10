#pragma once
#ifndef HEX_OBJECT_HPP_INCLUDED
#define HEX_OBJECT_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <vector>

#include "graphics.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "variant.hpp"
#include "hex_object_fwd.hpp"
#include "hex_map.hpp"
#include "hex_tile.hpp"

namespace hex {

class hex_map;

class hex_object : public game_logic::formula_callable
{
public:
	hex_object(const std::string& type, int x, int y, const hex_map* owner);
	virtual ~hex_object() {}

	virtual variant get_value(const std::string&) const;
	virtual void set_value(const std::string& key, const variant& value);

	virtual void draw() const;
	
	void build();
	void apply_rules(const std::string& rule);

	std::string type() const { return type_; }
	virtual bool execute_command(const variant& var);

	hex_object_ptr get_tile_in_dir(enum direction d) const;
	hex_object_ptr get_tile_in_dir(const std::string& s) const;

	int x() const { return x_; }
	int y() const { return y_; }

	hex_tile_ptr tile() const { return tile_->owner(); }

	static std::vector<std::string> get_rules();
	static std::vector<hex_tile_ptr> get_hex_tiles();
	static std::vector<hex_tile_ptr>& get_editor_tiles();

	static hex_tile_ptr get_hex_tile(const std::string& type);
private:

	// map coordinates.
	int x_;
	int y_;
	// Pointer to the tile in this square.
	const_basic_hex_tile_ptr tile_;
	// Transitions. mapping z-order to transition.
	std::vector<const_basic_hex_tile_ptr> transitions_;
	// String representing the base type of this tile.
	std::string type_;
	// raw pointer to the map that owns this.
	const hex_map* owner_map_;

	// Private default constructor and copy constructor to stop them
	// from being used.
	hex_object() {}
	hex_object(hex_object&) {}
};

}

#endif
