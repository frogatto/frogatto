#ifndef LEVEL_HPP_INCLUDED
#define LEVEL_HPP_INCLUDED

#include <bitset>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "boost/array.hpp"
#include "boost/scoped_ptr.hpp"

#include "background.hpp"
#include "character.hpp"
#include "entity.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "geometry.hpp"
#include "item.hpp"
#include "level_object.hpp"
#include "tile_map.hpp"
#include "wml_node_fwd.hpp"

class level : public game_logic::formula_callable
{
public:
	static const int TileSize = 32;
	explicit level(const std::string& level_cfg);

	//function to do anything which loads the level and must be done
	//in the main thread.
	void finish_loading();

	const std::string& replay_data() const { return replay_data_; }
	void load_save_point(const level& lvl);
	void set_save_point(int x, int y) { save_point_x_ = x; save_point_y_ = y; }

	const std::string& id() const { return id_; }
	const std::string& music() const { return music_; }

	wml::const_node_ptr write() const;
	void draw(int x, int y, int w, int h) const;
	void draw_background(double x, double y) const;
	void process();
	bool standable(int x, int y, int* friction=NULL, int* damage=NULL, int* adjust_y=NULL, entity_ptr* ch=NULL, const entity* exclude=NULL) const;
	bool solid(int x, int y, int* friction=NULL, int* damage=NULL) const;
	bool solid(const rect& r, int* friction=NULL, int* damage=NULL) const;
	entity_ptr collide(int x, int y, const entity* exclude=NULL) const;
	entity_ptr collide(const rect& r, const entity* exclude=NULL) const;
	character_ptr hit_by_player(const rect& r) const;
	const rect& boundaries() const { return boundaries_; }
	void set_boundaries(const rect& bounds) { boundaries_ = bounds; }
	void add_tile(const level_tile& t);
	void add_tile_rect(int zorder, int x1, int y1, int x2, int y2, const std::string& tile);
	void clear_tile_rect(int x1, int y1, int x2, int y2);
	void remove_tiles_at(int x, int y);
	const level_tile* get_tile_at(int x, int y) const;
	void remove_characters_in_rect(int x1, int y1, int x2, int y2);
	std::vector<entity_ptr> get_characters_in_rect(const rect& r) const;
	const_pc_character_ptr player() const { return player_; }
	pc_character_ptr player() { return player_; }
	void add_player(entity_ptr p);
	void add_character(entity_ptr p);
	void add_item(item_ptr p);

	struct portal {
		rect area;
		std::string level_dest;
		point dest;
		bool dest_starting_pos;
		bool automatic;
	};

	const portal* get_portal() const;
	void debug_dump_solid_map() const;

	int xscale() const { return xscale_; }
	int yscale() const { return yscale_; }

	int group_size(int group) const;
	void set_character_group(entity_ptr c, int group_num);
	int add_group();

	void set_editor() { editor_ = true; }

	void rebuild_tiles();

	const std::string& title() const { return title_; }

	int variations(int xtile, int ytile) const;
	void flip_variations(int xtile, int ytile);

	int auto_move_camera_x() const { return auto_move_camera_.x; }
	int auto_move_camera_y() const { return auto_move_camera_.y; }

	int air_resistance() const { return air_resistance_; }

	int camera_rotation() const;

	void set_end_game() { end_game_ = true; }
	bool end_game() const { return end_game_; }

private:
	void draw_layer(int layer, int x, int y, int w, int h) const;

	void rebuild_tiles_rect(const rect& r);
	void add_tile_solid(const level_tile& t);
	void add_solid_rect(int x1, int y1, int x2, int y2, int friction, int damage);
	void add_solid(int x, int y, int friction, int damage);
	void add_standable(int x, int y, int friction, int damage);
	typedef std::pair<int,int> tile_pos;
	typedef std::bitset<TileSize*TileSize> tile_bitmap;

	struct solid_info {
		solid_info() : all_solid(false), friction(0), damage(0)
		{}
		tile_bitmap bitmap;
		bool all_solid;
		int friction;
		int damage;
	};

	std::string id_;
	std::string music_;
	std::string replay_data_;
	int cycle_;

	typedef std::map<tile_pos, solid_info> solid_map;
	solid_map solid_;
	solid_map standable_;

	bool is_solid(const solid_map& map, int x, int y, int* friction, int* damage) const;
	void set_solid(solid_map& map, int x, int y, int friction, int damage);

	variant get_value(const std::string& key) const;

	std::string title_;

	rect boundaries_;

	struct solid_rect {
		rect r;
		int friction;
		int damage;
	};
	std::vector<solid_rect> solid_rects_;
	std::vector<level_tile> tiles_;
	std::set<int> layers_;
	std::vector<entity_ptr> chars_;
	std::vector<entity_ptr> active_chars_;
	pc_character_ptr player_;

	//characters stored in wml format; they can't be loaded in a separate thread
	//they will be loaded when complete_load_level() is called.
	std::vector<wml::const_node_ptr> wml_chars_;

	void load_character(wml::const_node_ptr c);

	typedef std::vector<entity_ptr> entity_group;
	std::vector<entity_group> groups_;

	std::vector<item_ptr> items_;
	std::vector<item_ptr> active_items_;

	std::vector<portal> portals_;
	boost::scoped_ptr<background> background_;
	int widest_tile_, highest_tile_;

	std::map<int, tile_map> tile_maps_;
	int xscale_, yscale_;

	int save_point_x_, save_point_y_;
	bool editor_;

	point auto_move_camera_;
	int air_resistance_;

	game_logic::const_formula_ptr camera_rotation_;
	bool end_game_;

	std::vector<std::string> preloads_; //future levels to preload
};

#endif
