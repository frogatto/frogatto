#ifndef TILE_MAP_HPP_INCLUDED
#define TILE_MAP_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/regex.hpp>

#include <map>
#include <string>

#include "formula_callable.hpp"
#include "geometry.hpp"
#include "level_object.hpp"
#include "wml_node_fwd.hpp"

struct tile_pattern;
struct multi_tile_pattern;

namespace {
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
	int x_speed() const { return x_speed_; }
	int y_speed() const { return y_speed_; }
	void set_zorder(int z) { zorder_ = z; }
	const char* get_tile_from_pixel_pos(int xpos, int ypos) const;
	const char* get_tile(int y, int x) const;
	int get_variations(int x, int y) const;
	void flip_variation(int x, int y, int delta=0);
private:
	void build_patterns();
	const std::vector<const tile_pattern*>& get_patterns() const;

	int variation(int x, int y) const;
	const tile_pattern* get_matching_pattern(int x, int y, tile_pattern_cache& cache, bool* face_right) const;
	variant get_value(const std::string& key) const { return variant(); }
	int xpos_, ypos_;
	int x_speed_, y_speed_;
	int zorder_;

	typedef boost::array<char, 4> tile_string;

	//a map of all of our strings, which maps into pattern_index.
	std::vector<std::vector<int> > map_;

	//an entry which holds one of the strings found in this map, as well
	//as the patterns it matches.
	struct pattern_index_entry {
		pattern_index_entry() { for(int n = 0; n != str.size(); ++n) { str[n] = 0; } }
		tile_string str;
		std::vector<const boost::regex*> matching_patterns;
	};

	const pattern_index_entry& get_tile_entry(int y, int x) const;

	std::vector<pattern_index_entry> pattern_index_;

	int get_pattern_index_entry(const tile_string& str);

	//the subset of all multi tile patterns which might be valid for this map.
	std::vector<const multi_tile_pattern*> multi_patterns_;

	void apply_matching_multi_pattern(int x, int y, const multi_tile_pattern& pattern, std::map<point, level_object_ptr>& mapping) const;

	//the subset of all global patterns which might be valid for this map.
	std::vector<const tile_pattern*> patterns_;

	//when we generate patterns_ we check the underlying vector's version.
	//when it is updated it will get a new version and so we'll have to
	//update our view into it.
	int patterns_version_;

	std::vector<std::vector<int> > variations_;
};

#endif
