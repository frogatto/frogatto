#ifndef TILE_MAP_HPP_INCLUDED
#define TILE_MAP_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/regex.hpp>

#include <map>
#include <string>

#include "formula_callable.hpp"
#include "geometry.hpp"
#include "level_object.hpp"
#include "point_map.hpp"
#include "variant.hpp"

struct tile_pattern;
struct multi_tile_pattern;

namespace {
struct tile_pattern_cache;
}

class tile_map : public game_logic::formula_callable {
public:
	static void init(variant node);
	static void load_all();
	static void load(const std::string& fname, const std::string& tile_id);
	static const std::vector<std::string>& get_files(const std::string& tile_id);

	tile_map();
	explicit tile_map(variant node);
	tile_map(const tile_map& o);
	~tile_map();

	variant write() const;
	void build_tiles(std::vector<level_tile>* tiles, const rect* r=NULL) const;
	bool set_tile(int xpos, int ypos, const std::string& str);
	int zorder() const { return zorder_; }
	int x_speed() const { return x_speed_; }
	int y_speed() const { return y_speed_; }
	void set_zorder(int z) { zorder_ = z; }
	void set_speed(int x_speed, int y_speed) { x_speed_ = x_speed; y_speed_ = y_speed; }
	const char* get_tile_from_pixel_pos(int xpos, int ypos) const;
	const char* get_tile(int y, int x) const;
	int get_variations(int x, int y) const;
	void flip_variation(int x, int y, int delta=0);

	//variants are not thread-safe, so this function clears out variant
	//info to prepare the tile map to be placed into a worker thread.
	void prepare_for_copy_to_worker_thread();

#ifndef NO_EDITOR
	//Functions for rebuilding all live tile maps when there is a change
	//to tile map data. prepare_rebuild_all() should be called before
	//the change, and rebuild_all() after.
	static void prepare_rebuild_all();
	static void rebuild_all();
#endif

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
		mutable std::vector<const boost::regex*> matching_patterns;
	};

	const pattern_index_entry& get_tile_entry(int y, int x) const;

	std::vector<pattern_index_entry> pattern_index_;

	int get_pattern_index_entry(const tile_string& str);

	//the subset of all multi tile patterns which might be valid for this map.
	std::vector<const multi_tile_pattern*> multi_patterns_;

	typedef std::pair<point, int> point_zorder;
	//function to apply the first found matching multi pattern.
	//mapping represents all the tiles added in our zorder.
	//different_zorder_mapping represents the mappings in different zorders
	//to this tile_map.
	void apply_matching_multi_pattern(int& x, int y,
	  const multi_tile_pattern& pattern,
	  point_map<level_object*>& mapping,
	  std::map<point_zorder, level_object*>& different_zorder_mapping) const;

	//the subset of all global patterns which might be valid for this map.
	std::vector<const tile_pattern*> patterns_;

	//when we generate patterns_ we check the underlying vector's version.
	//when it is updated it will get a new version and so we'll have to
	//update our view into it.
	int patterns_version_;

	std::vector<std::vector<int> > variations_;

#ifndef NO_EDITOR
	variant node_;
#endif
};

#endif
