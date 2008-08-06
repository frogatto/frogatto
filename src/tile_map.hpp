#ifndef TILE_MAP_HPP_INCLUDED
#define TILE_MAP_HPP_INCLUDED

#include <boost/array.hpp>

#include <map>
#include <string>

#include "formula_callable.hpp"
#include "geometry.hpp"
#include "level_object.hpp"
#include "wml_node_fwd.hpp"

namespace {
struct tile_pattern;
struct tile_pattern_cache;
}

class tile_map : public game_logic::formula_callable {
public:
	static void init(wml::const_node_ptr node);
	tile_map();
	explicit tile_map(wml::const_node_ptr node);
	wml::node_ptr write() const;
	void build_tiles(std::vector<level_tile>* tiles, const rect* r=NULL) const;
	bool set_tile(int xpos, int ypos, const std::string& str);
	int zorder() const { return zorder_; }
	void set_zorder(int z) { zorder_ = z; }
	const char* get_tile(int y, int x) const;
	int get_variations(int x, int y) const;
	void flip_variation(int x, int y);
private:
	int variation(int x, int y) const;
	const tile_pattern* get_matching_pattern(int x, int y, tile_pattern_cache& cache, bool* face_right) const;
	variant get_value(const std::string& key) const { return variant(); }
	int xpos_, ypos_;
	int zorder_;

	typedef boost::array<char, 4> tile_string;
	std::vector<std::vector<tile_string> > map_;

	std::vector<std::vector<int> > variations_;
};

#endif
