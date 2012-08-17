#pragma once
#ifndef HEX_TILE_PATTERN_HPP_INCLUDED
#define HEX_TILE_PATTERN_HPP_INCLUDED

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include <deque>
#include <string>
#include <vector>

#include "geometry.hpp"
#include "level_object.hpp"
#include "variant.hpp"

namespace hex {

class tile_pattern
{
public:
	static void load(variant node, const std::string& tile_id);
	tile_pattern(variant node, const std::string& tile_id);
	void init(variant node);

	int width()  const { return width_;  }
	int height() const { return height_; }

	int chance() const { return chance_; }

	const std::string& id() const { return id_; }

private:
	std::string default_tile_id_;
	std::string id_;
	int width_, height_;
	int chance_;
};

}

#endif // HEX_TILE_PATTERN_HPP_INCLUDED
