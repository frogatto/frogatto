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
#include "fluid.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "geometry.hpp"
#include "item.hpp"
#include "level_object.hpp"
#include "prop.hpp"
#include "tile_map.hpp"
#include "water.hpp"
#include "wml_node_fwd.hpp"
#include "color_utils.hpp"


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
	void draw_debug_solid(int x, int y, int w, int h) const;
	void draw_background(double x, double y, int rotation) const;
	void process();
	bool standable(int x, int y, int* friction=NULL, int* traction=NULL, int* damage=NULL, int* adjust_y=NULL, entity_ptr* ch=NULL, const entity* exclude=NULL) const;
	bool solid(int x, int y, int* friction=NULL, int* traction=NULL, int* damage=NULL) const;
	bool solid(const rect& r, int* friction=NULL, int* traction=NULL, int* damage=NULL) const;
	entity_ptr collide(int x, int y, const entity* exclude=NULL) const;
	entity_ptr collide(const rect& r, const entity* exclude=NULL) const;
	entity_ptr board(int x, int y) const;
	character_ptr hit_by_player(const rect& r) const;
	const rect& boundaries() const { return boundaries_; }
	void set_boundaries(const rect& bounds) { boundaries_ = bounds; }
	void add_tile(const level_tile& t);
	void add_tile_rect(int zorder, int x1, int y1, int x2, int y2, const std::string& tile);
	void add_tile_rect_vector(int zorder, int x1, int y1, int x2, int y2, const std::vector<std::string>& tiles);
	void get_tile_rect(int zorder, int x1, int y1, int x2, int y2, std::vector<std::string>& tiles) const;
	void get_all_tiles_rect(int x1, int y1, int x2, int y2, std::map<int, std::vector<std::string> >& tiles) const;
	void clear_tile_rect(int x1, int y1, int x2, int y2);
	void remove_tiles_at(int x, int y);
	const level_tile* get_tile_at(int x, int y) const;
	void remove_characters_in_rect(int x1, int y1, int x2, int y2);
	void remove_character(entity_ptr e);
	std::vector<entity_ptr> get_characters_in_rect(const rect& r) const;
	entity_ptr get_character_at_point(int x, int y) const;
	const_pc_character_ptr player() const { return player_; }
	pc_character_ptr player() { return player_; }
	void add_player(entity_ptr p);
	void add_character(entity_ptr p);
	void add_item(item_ptr p);
	void add_prop(const prop_object& new_prop);
	void remove_prop(const prop_object& new_prop);
	void get_props_in_rect(int x1, int y1, int x2, int y2, std::vector<prop_object>& props);
	void remove_props_in_rect(int x1, int y1, int x2, int y2);

	struct portal {
		rect area;
		std::string level_dest;
		std::string dest_str;
		point dest;
		bool dest_starting_pos;
		bool automatic;
		std::string transition;
	};

	//function which will make it so the next call to get_portal() will return
	//a pointer to a copy of the given portal. i.e. this makes the character immediately
	//enter a portal.
	void force_enter_portal(const portal& p);

	//the portal the character has entered (if any)
	const portal* get_portal() const;
	void debug_dump_solid_map() const;

	int xscale() const { return xscale_; }
	int yscale() const { return yscale_; }

	int group_size(int group) const;
	void set_character_group(entity_ptr c, int group_num);
	int add_group();

	void set_editor() { editor_ = true; }
	void set_editor_selection(entity_ptr c) { editor_selection_ = c; }
	entity_ptr editor_selection() const { return editor_selection_; }

	void rebuild_tiles();

	const std::string& title() const { return title_; }

	int variations(int xtile, int ytile) const;
	void flip_variations(int xtile, int ytile, int delta=1);

	int auto_move_camera_x() const { return auto_move_camera_.x; }
	int auto_move_camera_y() const { return auto_move_camera_.y; }

	int air_resistance() const { return air_resistance_; }
	int water_resistance() const { return water_resistance_; }

	int camera_rotation() const;

	void set_end_game() { end_game_ = true; }
	bool end_game() const { return end_game_; }

	//Function used when the player is entering the level at a certain point.
	//Will take the name of a destination within a level and return the point
	//at that location. For now only takes "left" and "right".
	point get_dest_from_str(const std::string& key) const;

	//levels ended up at by exiting this level to the left or right.
	const std::string& previous_level() const;
	const std::string& next_level() const;

	void set_previous_level(const std::string& name);
	void set_next_level(const std::string& name);

	int cycle() const { return cycle_; }
	bool is_underwater(const rect& r, rect* res_water_area=NULL) const;

	void get_current(const entity& e, int* velocity_x, int* velocity_y) const;

	bool hide_status_bar() const { return hide_status_bar_; }
	graphics::color tint() const { return tint_; }

	water* get_water() { return water_.get(); }
	const water* get_water() const { return water_.get(); }

	water& get_or_create_water();

private:
	bool add_tile_rect_vector_internal(int zorder, int x1, int y1, int x2, int y2, const std::vector<std::string>& tiles);

	void draw_layer(int layer, int x, int y, int w, int h) const;

	void rebuild_tiles_rect(const rect& r);
	void add_tile_solid(const level_tile& t);
	void add_solid_rect(int x1, int y1, int x2, int y2, int friction, int traction, int damage);
	void add_solid(int x, int y, int friction, int traction, int damage);
	void add_standable(int x, int y, int friction, int traction, int damage);
	typedef std::pair<int,int> tile_pos;
	typedef std::bitset<TileSize*TileSize> tile_bitmap;

	struct solid_info {
		solid_info() : all_solid(false), friction(0), traction(0), damage(0)
		{}
		tile_bitmap bitmap;
		bool all_solid;
		int friction;
		int traction;
		int damage;
	};

	std::string id_;
	std::string music_;
	std::string replay_data_;
	int cycle_;

	std::map<std::string, variant> vars_;
	
	typedef std::map<tile_pos, solid_info> solid_map;
	solid_map solid_;
	solid_map standable_;

	bool is_solid(const solid_map& map, int x, int y, int* friction, int* traction, int* damage) const;
	void set_solid(solid_map& map, int x, int y, int friction, int traction, int damage);

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	std::string title_;

	rect boundaries_;

	struct solid_rect {
		rect r;
		int friction;
		int traction;
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

	std::vector<prop_object> props_;

	portal left_portal_, right_portal_;
	std::vector<portal> portals_;

	mutable bool entered_portal_active_;
	portal entered_portal_;

	boost::scoped_ptr<background> background_;
	int widest_tile_, highest_tile_;

	std::map<int, tile_map> tile_maps_;
	int xscale_, yscale_;

	int save_point_x_, save_point_y_;
	bool editor_;
	entity_ptr editor_selection_;

	point auto_move_camera_;
	int air_resistance_;
	int water_resistance_;

	game_logic::const_formula_ptr camera_rotation_;
	bool end_game_;

	std::vector<std::string> preloads_; //future levels to preload

	boost::scoped_ptr<fluid> fluid_;

	boost::scoped_ptr<water> water_;

	bool hide_status_bar_;
	graphics::color tint_;
	
};

#endif
