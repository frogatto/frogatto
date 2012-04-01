#include <boost/bind.hpp>

#include <algorithm>
#include <iostream>
#include <math.h>

#include "IMG_savepng.h"
#include "asserts.hpp"
#include "collision_utils.hpp"
#include "controls.hpp"
#include "draw_scene.hpp"
#include "draw_tile.hpp"
#include "editor.hpp"
#include "entity.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "gui_formula_functions.hpp"
#include "iphone_controls.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "light.hpp"
#include "load_level.hpp"
#include "multiplayer.hpp"
#include "object_events.hpp"
#include "player_info.hpp"
#include "playable_custom_object.hpp"
#include "preferences.hpp"
#include "preprocessor.hpp"
#include "random.hpp"
#include "raster.hpp"
#include "sound.hpp"
#include "stats.hpp"
#include "string_utils.hpp"
#include "surface_palette.hpp"
#include "texture_frame_buffer.hpp"
#include "thread.hpp"
#include "tile_map.hpp"
#include "unit_test.hpp"
#include "variant_utils.hpp"
#include "wml_formula_callable.hpp"
#include "color_utils.hpp"

#include "compat.hpp"

namespace {
boost::intrusive_ptr<level> current_level;

std::map<std::string, level::summary> load_level_summaries() {
	std::map<std::string, level::summary> result;
	const variant node = json::parse_from_file("data/compiled/level_index.cfg");
	
	foreach(variant level_node, node["level"].as_list()) {
		level::summary& s = result[level_node["level"].as_string()];
		s.music = level_node["music"].as_string();
		s.title = level_node["title"].as_string();
	}

	return result;
}

bool level_tile_not_in_rect(const rect& r, const level_tile& t) {
	return t.x < r.x() || t.y < r.y() || t.x >= r.x2() || t.y >= r.y2();
}

}

void level::clear_current_level()
{
	current_level.reset();
}

level::summary level::get_summary(const std::string& id)
{
	static const std::map<std::string, summary> summaries = load_level_summaries();
	std::map<std::string, summary>::const_iterator i = summaries.find(id);
	if(i != summaries.end()) {
		return i->second;
	}

	return summary();
}

level& level::current()
{
	ASSERT_LOG(current_level, "Tried to query current level when there is none");
	return *current_level;
}

level* level::current_ptr()
{
	return current_level.get();
}

void level::set_as_current_level()
{
	current_level = this;
	frame::set_color_palette(palettes_used_);

#if !TARGET_OS_IPHONE && !TARGET_BLACKBERRY
	static const int starting_x_resolution = preferences::actual_screen_width();
	static const int starting_y_resolution = preferences::actual_screen_height();
	static const int starting_virtual_x_resolution = preferences::virtual_screen_width();
	static const int starting_virtual_y_resolution = preferences::virtual_screen_height();

	if(!editor_ && !editor_resolution_manager::is_active() && starting_x_resolution == starting_virtual_x_resolution) {
		if(!x_resolution_) {
			x_resolution_ = starting_x_resolution;
		}

		if(!y_resolution_) {
			y_resolution_ = starting_y_resolution;
		}


		if(x_resolution_ != preferences::actual_screen_width() || y_resolution_ != preferences::actual_screen_height()) {

			std::cerr << "RESETTING VIDEO MODE: " << x_resolution_ << ", " << y_resolution_ << "\n";
			const bool result = graphics::set_video_mode(x_resolution_, y_resolution_);
			if(result) {
				preferences::set_actual_screen_width(x_resolution_);
				preferences::set_actual_screen_height(y_resolution_);
				preferences::set_virtual_screen_width(x_resolution_);
				preferences::set_virtual_screen_height(y_resolution_);
			}
		}
	}
	
#endif
}

namespace {
graphics::color_transform default_dark_color() {
	return graphics::color_transform(0, 0, 0, 0);
}
}

level::level(const std::string& level_cfg, variant node)
	: id_(level_cfg),
	  x_resolution_(0), y_resolution_(0),
	  highlight_layer_(INT_MIN),
	  num_compiled_tiles_(0),
	  entered_portal_active_(false), save_point_x_(-1), save_point_y_(-1),
	  editor_(false), show_foreground_(true), show_background_(true), dark_(false), dark_color_(graphics::color_transform(0, 0, 0, 255)), air_resistance_(0), water_resistance_(7), end_game_(false),
      editor_tile_updates_frozen_(0), zoom_level_(decimal::from_int(1)),
	  palettes_used_(0),
	  background_palette_(-1),
	  segment_width_(0), segment_height_(0)
{
	std::cerr << "in level constructor...\n";
	const int start_time = SDL_GetTicks();

	if(node.is_null()) {
		node = load_level_wml(level_cfg);
	}

	variant player_save_node;
	ASSERT_LOG(node.is_null() == false, "LOAD LEVEL WML FOR " << level_cfg << " FAILED");
	if(node.has_key("id")) {
		id_ = node["id"].as_string();
	}

	if(preferences::load_compiled() && (level_cfg == "save.cfg" || level_cfg == "autosave.cfg")) {
		if(preferences::version() != node["version"].as_string()) {
			std::cerr << "DIFFERENT VERSION LEVEL\n";
			foreach(variant obj_node, node["character"].as_list()) {
				if(obj_node["is_human"].as_bool(false)) {
					player_save_node = obj_node;
					break;
				}
			}

			variant n = node;
			if(node.has_key("id")) {
				n = load_level_wml(node["id"].as_string());
			}

			n = n.add_attr(variant("serialized_objects"), n["serialized_objects"] + node["serialized_objects"]);

			node = n;
		}
	}

	dark_color_ = default_dark_color();
	if(node["dark"].as_bool(false)) {
		dark_ = true;
	}

	if(node.has_key("dark_color")) {
		dark_color_ = graphics::color_transform(node["dark_color"]);
	}

	vars_ = node["vars"];
	if(vars_.is_map() == false) {
		std::map<variant,variant> m;
		vars_ = variant(&m);
	}

	segment_width_ = node["segment_width"].as_int();
	ASSERT_LOG(segment_width_%32 == 0, "segment_width in " << id_ << " is not divisible by 32");

	segment_height_ = node["segment_height"].as_int();
	ASSERT_LOG(segment_height_%32 == 0, "segment_height in " << id_ << " is not divisible by 32");

	music_ = node["music"].as_string_default();
	replay_data_ = node["replay_data"].as_string_default();
	cycle_ = node["cycle"].as_int();
	time_freeze_ = 0;
	x_resolution_ = node["x_resolution"].as_int();
	y_resolution_ = node["y_resolution"].as_int();
	in_dialog_ = false;
	title_ = node["title"].as_string_default();
	if(node.has_key("dimensions")) {
		boundaries_ = rect(node["dimensions"]);
	} else {
		boundaries_ = rect(0, 0, node["width"].as_int(800), node["height"].as_int(600));
	}

	if(node.has_key("lock_screen")) {
		lock_screen_.reset(new point(node["lock_screen"].as_string()));
	}

	if(node.has_key("opaque_rects")) {
		const std::vector<std::string> opaque_rects_str = util::split(node["opaque_rects"].as_string(), ':');
		foreach(const std::string& r, opaque_rects_str) {
			opaque_rects_.push_back(rect(r));
			std::cerr << "OPAQUE RECT: " << r << "\n";
		}
	}

	xscale_ = node["xscale"].as_int(100);
	yscale_ = node["yscale"].as_int(100);
	auto_move_camera_ = point(node["auto_move_camera"]);
	air_resistance_ = node["air_resistance"].as_int(20);
	water_resistance_ = node["water_resistance"].as_int(100);

	camera_rotation_ = game_logic::formula::create_optional_formula(node["camera_rotation"]);

	preloads_ = util::split(node["preloads"].as_string());

	std::string empty_solid_info;
	foreach(variant rect_node, node["solid_rect"].as_list()) {
		solid_rect r;
		r.r = rect(rect_node["rect"]);
		r.friction = rect_node["friction"].as_int(100);
		r.traction = rect_node["traction"].as_int(100);
		r.damage = rect_node["damage"].as_int();
		solid_rects_.push_back(r);
		add_solid_rect(r.r.x(), r.r.y(), r.r.x2(), r.r.y2(), r.friction, r.traction, r.damage, empty_solid_info);
	}

	std::cerr << "building..." << SDL_GetTicks() << "\n";
	widest_tile_ = 0;
	highest_tile_ = 0;
	layers_.insert(0);
	foreach(variant tile_node, node["tile"].as_list()) {
		const level_tile t = level_object::build_tile(tile_node);
		tiles_.push_back(t);
		layers_.insert(t.zorder);
		add_tile_solid(t);
	}
	std::cerr << "done building..." << SDL_GetTicks() << "\n";

	int begin_tile_index = tiles_.size();
	foreach(variant tile_node, node["tile_map"].as_list()) {
		tile_map m(tile_node);
		tile_maps_[m.zorder()] = m;
		const int before = tiles_.size();
		tile_maps_[m.zorder()].build_tiles(&tiles_);
		std::cerr << "LAYER " << m.zorder() << " BUILT " << (tiles_.size() - before) << " tiles\n";
	}

	std::cerr << "done building tile_map..." << SDL_GetTicks() << "\n";

	num_compiled_tiles_ = node["num_compiled_tiles"].as_int();

	tiles_.resize(tiles_.size() + num_compiled_tiles_);
	std::vector<level_tile>::iterator compiled_itor = tiles_.end() - num_compiled_tiles_;

	foreach(variant tile_node, node["compiled_tiles"].as_list()) {
		read_compiled_tiles(tile_node, compiled_itor);
		wml_compiled_tiles_.push_back(tile_node);
	}

	ASSERT_LOG(compiled_itor == tiles_.end(), "INCORRECT NUMBER OF COMPILED TILES");

	for(int i = begin_tile_index; i != tiles_.size(); ++i) {
		add_tile_solid(tiles_[i]);
		layers_.insert(tiles_[i].zorder);
	}

	if(std::adjacent_find(tiles_.rbegin(), tiles_.rend(), level_tile_zorder_pos_comparer()) != tiles_.rend()) {
		std::sort(tiles_.begin(), tiles_.end(), level_tile_zorder_pos_comparer());
	}

	if(node.has_key("palettes")) {
		std::vector<std::string> v = util::split(node["palettes"].as_string());
		foreach(const std::string& p, v) {
			const int id = graphics::get_palette_id(p);
			palettes_used_ |= (1 << id);
		}
	}

	if(node.has_key("background_palette")) {
		background_palette_ = graphics::get_palette_id(node["background_palette"].as_string());
	}

	prepare_tiles_for_drawing();

	foreach(variant char_node, node["character"].as_list()) {
		if(player_save_node.is_null() == false && char_node["is_human"].as_bool(false)) {
			continue;
		}

		wml_chars_.push_back(char_node);
		continue;
	}

	if(player_save_node.is_null() == false) {
		wml_chars_.push_back(player_save_node);
	}

	variant serialized_objects = node["serialized_objects"];
	if(serialized_objects.is_null() == false) {
		serialized_objects_.push_back(serialized_objects);
	}

	foreach(variant portal_node, node["portal"].as_list()) {
		portal p;
		p.area = rect(portal_node["rect"]);
		p.level_dest = portal_node["level"].as_string();
		p.dest = point(portal_node["dest"].as_string());
		p.dest_starting_pos = portal_node["dest_starting_post"].as_bool(false);
		p.automatic = portal_node["automatic"].as_bool(true);
		p.transition = portal_node["transition"].as_string();
		portals_.push_back(p);
	}

	if(node.has_key("next_level")) {
		right_portal_.level_dest = node["next_level"].as_string();
		right_portal_.dest_str = "left";
		right_portal_.dest_starting_pos = false;
		right_portal_.automatic = true;
	}

	if(node.has_key("previous_level")) {
		left_portal_.level_dest = node["previous_level"].as_string();
		left_portal_.dest_str = "right";
		left_portal_.dest_starting_pos = false;
		left_portal_.automatic = true;
	}

	variant bg = node["background"];
	if(bg.is_map()) {
		background_.reset(new background(bg, background_palette_));
	} else if(node.has_key("background")) {
		background_ = background::get(node["background"].as_string(), background_palette_);
		background_offset_ = point(node["background_offset"]);
		background_->set_offset(background_offset_);
	}

	if(node.has_key("water")) {
		water_.reset(new water(node["water"]));
	}

	foreach(variant script_node, node["script"].as_list()) {
		movement_script s(script_node);
		movement_scripts_[s.id()] = s;
	}

	gui_algo_str_ = node.has_key("gui") ? node["gui"].as_string() : "default";
	gui_algorithm_ = gui_algorithm::get(gui_algo_str_);
	gui_algorithm_->new_level();

	sub_level_str_ = node["sub_levels"].as_string_default();
	foreach(const std::string& sub_lvl, util::split(sub_level_str_)) {
		sub_level_data& data = sub_levels_[sub_lvl];
		data.lvl = boost::intrusive_ptr<level>(new level(sub_lvl + ".cfg"));
		foreach(int layer, data.lvl->layers_) {
			layers_.insert(layer);
		}

		data.active = false;
		data.xoffset = data.yoffset = 0;
		data.xbase = data.ybase = 0;
	}

	const int time_taken_ms = (SDL_GetTicks() - start_time);
	stats::entry("load", id()).set("time", variant(time_taken_ms));
	std::cerr << "done level constructor: " << time_taken_ms << "\n";
}

level::~level()
{
}

void level::read_compiled_tiles(variant node, std::vector<level_tile>::iterator& out)
{
	const int xbase = node["x"].as_int();
	const int ybase = node["y"].as_int();
	const int zorder = node["zorder"].as_int();

	int x = xbase;
	int y = ybase;
	const std::string& tiles = node["tiles"].as_string();
	const char* i = tiles.c_str();
	const char* end = tiles.c_str() + tiles.size();
	while(i != end) {
		if(*i == '|') {
			++i;
		} else if(*i == ',') {
			x += TileSize;
			++i;
		} else if(*i == '\n') {
			x = xbase;
			y += TileSize;
			++i;
		} else {
			ASSERT_LOG(out != tiles_.end(), "NOT ENOUGH COMPILED TILES REPORTED");

			out->x = x;
			out->y = y;
			out->zorder = zorder;
			out->face_right = false;
			out->draw_disabled = false;
			if(*i == '~') {
				out->face_right = true;
				++i;
			}

			ASSERT_LOG(end - i >= 3, "ILLEGAL TILE FOUND");

			out->object = level_object::get_compiled(i);
			++out;
			i += 3;
		}
	}
}

void level::load_character(variant c)
{
	chars_.push_back(entity::build(c));
	layers_.insert(chars_.back()->zorder());
	if(!chars_.back()->is_human()) {
		chars_.back()->set_id(chars_.size());
	}
	if(chars_.back()->is_human()) {
		if(players_.size() == multiplayer::slot()) {
			last_touched_player_ = player_ = chars_.back();
		}

		players_.push_back(chars_.back());
		players_.back()->get_player_info()->set_player_slot(players_.size() - 1);
	}

	const int group = chars_.back()->group();
	if(group >= 0) {
		if(group >= groups_.size()) {
			groups_.resize(group + 1);
		}

		groups_[group].push_back(chars_.back());
	}

	if(chars_.back()->label().empty() == false) {
		chars_by_label_[chars_.back()->label()] = chars_.back();
	}

	solid_chars_.clear();
}

void level::finish_loading()
{
	std::vector<sub_level_data> sub_levels;
	if((segment_width_ > 0 || segment_height_ > 0) && !editor_ && !preferences::compiling_tiles) {

		const int seg_width = segment_width_ > 0 ? segment_width_ : boundaries_.w();
		const int seg_height = segment_height_ > 0 ? segment_height_ : boundaries_.h();

		for(int y = boundaries_.y(); y < boundaries_.y2(); y += seg_height) {
			for(int x = boundaries_.x(); x < boundaries_.x2(); x += seg_width) {
				level* sub_level = new level(*this);
				const rect bounds(x, y, seg_width, seg_height);

				sub_level->boundaries_ = bounds;
				sub_level->tiles_.erase(std::remove_if(sub_level->tiles_.begin(), sub_level->tiles_.end(), boost::bind(level_tile_not_in_rect, bounds, _1)), sub_level->tiles_.end());
				sub_level->solid_.clear();
				sub_level->standable_.clear();
				foreach(const level_tile& t, sub_level->tiles_) {
					sub_level->add_tile_solid(t);
				}
				sub_level->prepare_tiles_for_drawing();

				sub_level_data data;
				data.lvl.reset(sub_level);
				data.xbase = x;
				data.ybase = y;
				data.xoffset = data.yoffset = 0;
				data.active = false;
				sub_levels.push_back(data);
			}
		}

		const std::vector<entity_ptr> objects = get_chars();
		foreach(const entity_ptr& obj, objects) {
			if(!obj->is_human()) {
				remove_character(obj);
			}
		}

		solid_.clear();
		standable_.clear();
		tiles_.clear();
		prepare_tiles_for_drawing();

		int index = 0;
		foreach(const sub_level_data& data, sub_levels) {
			sub_levels_[formatter() << index] = data;
			++index;
		}
	}

	if(sub_levels_.empty() == false) {
		solid_base_ = solid_;
		standable_base_ = standable_;
	}

	graphics::texture::build_textures_from_worker_threads();

	if (editor_ || preferences::compiling_tiles)
		game_logic::set_verbatim_string_expressions (true);

	std::vector<entity_ptr> objects_not_in_level;

	{
	game_logic::wml_formula_callable_read_scope read_scope;
	foreach(variant node, serialized_objects_) {
		foreach(variant obj_node, node["character"].as_list()) {
			const intptr_t addr_id = strtoll(obj_node["_addr"].as_string().c_str(), NULL, 16);
			entity_ptr obj(entity::build(obj_node));
			objects_not_in_level.push_back(obj);
			game_logic::wml_formula_callable_read_scope::register_serialized_object(addr_id, obj);
			fprintf(stderr, "DESERIALIZED: %x\n", addr_id);
		}
	}

	foreach(variant node, wml_chars_) {
		load_character(node);

		const intptr_t addr_id = strtoll(node["_addr"].as_string().c_str(), NULL, 16);
		game_logic::wml_formula_callable_read_scope::register_serialized_object(addr_id, chars_.back());

		if(node.has_key("attached_objects")) {
			std::cerr << "LOADING ATTACHED: " << node["attached_objects"].as_string() << "\n";
			std::vector<entity_ptr> attached;
			std::vector<std::string> v = util::split(node["attached_objects"].as_string());
			foreach(const std::string& s, v) {
				std::cerr << "ATTACHED: " << s << "\n";
				const intptr_t addr_id = strtoll(s.c_str(), NULL, 16);
				game_logic::wml_serializable_formula_callable_ptr obj = game_logic::wml_formula_callable_read_scope::get_serialized_object(addr_id);
				entity* e = dynamic_cast<entity*>(obj.get());
				if(e) {
					std::cerr << "GOT ATTACHED\n";
					attached.push_back(entity_ptr(e));
				}
			}

			chars_.back()->set_attached_objects(attached);
		}
	}

	game_logic::set_verbatim_string_expressions (false);

	wml_chars_.clear();
	serialized_objects_.clear();

	controls::new_level(cycle_, players_.empty() ? 1 : players_.size(), multiplayer::slot());

	//start loading FML for previous and next level
	if(!previous_level().empty()) {
		preload_level_wml(previous_level());
	}

	if(!next_level().empty()) {
		preload_level_wml(next_level());
	}

	if(!sub_levels.empty()) {
		const int seg_width = segment_width_ > 0 ? segment_width_ : boundaries_.w();
		const int seg_height = segment_height_ > 0 ? segment_height_ : boundaries_.h();
		int segment_number = 0;
		for(int y = boundaries_.y(); y < boundaries_.y2(); y += seg_height) {
			for(int x = boundaries_.x(); x < boundaries_.x2(); x += seg_width) {
				const std::vector<entity_ptr> objects = get_chars();
				foreach(const entity_ptr& obj, objects) {
					if(!obj->is_human() && obj->midpoint().x >= x && obj->midpoint().x < x + seg_width && obj->midpoint().y >= y && obj->midpoint().y < y + seg_height) {
						ASSERT_INDEX_INTO_VECTOR(segment_number, sub_levels);
						sub_levels[segment_number].lvl->add_character(obj);
						remove_character(obj);
					}
				}

				++segment_number;
			}
		}
	}

	} //end serialization read scope. Now all objects should be fully resolved.

	//iterate over all our objects and let them do any final loading actions.
	foreach(entity_ptr e, objects_not_in_level) {
		if(e) {
			e->finish_loading();
		}
	}

	foreach(entity_ptr e, chars_) {
		if(e) {
			e->finish_loading();
		}
	}
	
}

void level::set_multiplayer_slot(int slot)
{
	ASSERT_INDEX_INTO_VECTOR(slot, players_);
	last_touched_player_ = player_ = players_[slot];
	controls::new_level(cycle_, players_.empty() ? 1 : players_.size(), slot);
}

void level::load_save_point(const level& lvl)
{
	if(lvl.save_point_x_ < 0) {
		return;
	}

	save_point_x_ = lvl.save_point_x_;
	save_point_y_ = lvl.save_point_y_;
	if(player_) {
		player_->set_pos(save_point_x_, save_point_y_);
	}
}

namespace {
//we allow rebuilding tiles in the background. We only rebuild the tiles
//one at a time, if more requests for rebuilds come in while we are
//rebuilding, then queue the requests up.

//the level we're currently building tiles for.
const level* level_building = NULL;

struct level_tile_rebuild_info {
	level_tile_rebuild_info() : tile_rebuild_in_progress(false),
	                            tile_rebuild_queued(false),
								rebuild_tile_thread(NULL),
								tile_rebuild_complete(false)
	{}

	//record whether we are currently rebuilding tiles, and if we have had
	//another request come in during the current building of tiles.
	bool tile_rebuild_in_progress;
	bool tile_rebuild_queued;

	threading::thread* rebuild_tile_thread;

	//an unsynchronized buffer only accessed by the main thread with layers
	//that will be rebuilt.
	std::vector<int> rebuild_tile_layers_buffer;

	//buffer accessed by the worker thread which contains layers that will
	//be rebuilt.
	std::vector<int> rebuild_tile_layers_worker_buffer;

	//a locked flag which is polled to see if tile rebuilding has been completed.
	bool tile_rebuild_complete;

	threading::mutex tile_rebuild_complete_mutex;

	//the tiles where the thread will store the new tiles.
	std::vector<level_tile> task_tiles;
};

std::map<const level*, level_tile_rebuild_info> tile_rebuild_map;

void build_tiles_thread_function(level_tile_rebuild_info* info, std::map<int, tile_map> tile_maps) {
	info->task_tiles.clear();

	if(info->rebuild_tile_layers_worker_buffer.empty()) {
		for(std::map<int, tile_map>::const_iterator i = tile_maps.begin();
		    i != tile_maps.end(); ++i) {
			i->second.build_tiles(&info->task_tiles);
		}
	} else {
		foreach(int layer, info->rebuild_tile_layers_worker_buffer) {
			std::map<int, tile_map>::const_iterator itor = tile_maps.find(layer);
			if(itor != tile_maps.end()) {
				itor->second.build_tiles(&info->task_tiles);
			}
		}
	}

	threading::lock l(info->tile_rebuild_complete_mutex);
	info->tile_rebuild_complete = true;
}

}

void level::start_rebuild_tiles_in_background(const std::vector<int>& layers)
{
	level_tile_rebuild_info& info = tile_rebuild_map[this];

	//merge the new layers with any layers we already have queued up.
	if(layers.empty() == false && (!info.tile_rebuild_queued || info.rebuild_tile_layers_buffer.empty() == false)) {
		//add the layers we want to rebuild to those already requested.
		info.rebuild_tile_layers_buffer.insert(info.rebuild_tile_layers_buffer.end(), layers.begin(), layers.end());
		std::sort(info.rebuild_tile_layers_buffer.begin(), info.rebuild_tile_layers_buffer.end());
		info.rebuild_tile_layers_buffer.erase(std::unique(info.rebuild_tile_layers_buffer.begin(), info.rebuild_tile_layers_buffer.end()), info.rebuild_tile_layers_buffer.end());
	} else if(layers.empty()) {
		info.rebuild_tile_layers_buffer.clear();
	}

	if(info.tile_rebuild_in_progress) {
		info.tile_rebuild_queued = true;
		return;
	}

	info.tile_rebuild_in_progress = true;
	info.tile_rebuild_complete = false;

	info.rebuild_tile_layers_worker_buffer = info.rebuild_tile_layers_buffer;
	info.rebuild_tile_layers_buffer.clear();

	info.rebuild_tile_thread = new threading::thread(boost::bind(build_tiles_thread_function, &info, tile_maps_));
}

void level::freeze_rebuild_tiles_in_background()
{
	level_tile_rebuild_info& info = tile_rebuild_map[this];
	info.tile_rebuild_in_progress = true;
}

void level::unfreeze_rebuild_tiles_in_background()
{
	level_tile_rebuild_info& info = tile_rebuild_map[this];
	if(info.rebuild_tile_thread != NULL) {
		//a thread is actually in flight calculating tiles, so any requests
		//would have been queued up anyway.
		return;
	}

	info.tile_rebuild_in_progress = false;
	start_rebuild_tiles_in_background(info.rebuild_tile_layers_buffer);
}

namespace {
bool level_tile_from_layer(const level_tile& t, int zorder) {
	return t.layer_from == zorder;
}
}

void level::complete_rebuild_tiles_in_background()
{
	level_tile_rebuild_info& info = tile_rebuild_map[this];
	if(!info.tile_rebuild_in_progress) {
		return;
	}

	{
		threading::lock l(info.tile_rebuild_complete_mutex);
		if(!info.tile_rebuild_complete) {
			return;
		}
	}

	const int begin_time = SDL_GetTicks();

//	ASSERT_LOG(rebuild_tile_thread, "REBUILD TILE THREAD IS NULL");
	delete info.rebuild_tile_thread;
	info.rebuild_tile_thread = NULL;

	if(info.rebuild_tile_layers_worker_buffer.empty()) {
		tiles_.clear();
	} else {
		foreach(int layer, info.rebuild_tile_layers_worker_buffer) {
			tiles_.erase(std::remove_if(tiles_.begin(), tiles_.end(), boost::bind(level_tile_from_layer, _1, layer)), tiles_.end());
		}
	}

	tiles_.insert(tiles_.end(), info.task_tiles.begin(), info.task_tiles.end());
	info.task_tiles.clear();

	complete_tiles_refresh();

	std::cerr << "COMPLETE TILE REBUILD: " << (SDL_GetTicks() - begin_time) << "\n";

	info.rebuild_tile_layers_worker_buffer.clear();

	info.tile_rebuild_in_progress = false;
	if(info.tile_rebuild_queued) {
		info.tile_rebuild_queued = false;
		start_rebuild_tiles_in_background(info.rebuild_tile_layers_buffer);
	}
}

void level::rebuild_tiles()
{
	if(editor_tile_updates_frozen_) {
		return;
	}

	tiles_.clear();
	for(std::map<int, tile_map>::const_iterator i = tile_maps_.begin(); i != tile_maps_.end(); ++i) {
		i->second.build_tiles(&tiles_);
	}

	complete_tiles_refresh();
}

void level::complete_tiles_refresh()
{
	const int start = SDL_GetTicks();
	std::cerr << "adding solids..." << (SDL_GetTicks() - start) << "\n";
	solid_.clear();
	standable_.clear();

	foreach(level_tile& t, tiles_) {
		add_tile_solid(t);
		layers_.insert(t.zorder);
	}

	std::cerr << "sorting..." << (SDL_GetTicks() - start) << "\n";

	if(std::adjacent_find(tiles_.rbegin(), tiles_.rend(), level_tile_zorder_pos_comparer()) != tiles_.rend()) {
		std::sort(tiles_.begin(), tiles_.end(), level_tile_zorder_pos_comparer());
	}
	prepare_tiles_for_drawing();
	std::cerr << "done..." << (SDL_GetTicks() - start) << "\n";
}

int level::variations(int xtile, int ytile) const
{
	for(std::map<int, tile_map>::const_iterator i = tile_maps_.begin();
	    i != tile_maps_.end(); ++i) {
		const int var = i->second.get_variations(xtile, ytile);
		if(var > 1) {
			return var;
		}
	}

	return 1;
}

void level::flip_variations(int xtile, int ytile, int delta)
{
	for(std::map<int, tile_map>::iterator i = tile_maps_.begin();
	    i != tile_maps_.end(); ++i) {
		std::cerr << "get_variations zorder: " << i->first << "\n";
		if(i->second.get_variations(xtile, ytile) > 1) {
			i->second.flip_variation(xtile, ytile, delta);
		}
	}

	rebuild_tiles_rect(rect(xtile*TileSize, ytile*TileSize, TileSize, TileSize));
}

namespace {
struct TileInRect {
	explicit TileInRect(const rect& r) : rect_(r)
	{}

	bool operator()(const level_tile& t) const {
		return point_in_rect(point(t.x, t.y), rect_);
	}

	rect rect_;
};
}

void level::rebuild_tiles_rect(const rect& r)
{
	if(editor_tile_updates_frozen_) {
		return;
	}

	for(int x = r.x(); x < r.x2(); x += TileSize) {
		for(int y = r.y(); y < r.y2(); y += TileSize) {
			tile_pos pos(x/TileSize, y/TileSize);
			solid_.erase(pos);
			standable_.erase(pos);
		}
	}

	tiles_.erase(std::remove_if(tiles_.begin(), tiles_.end(), TileInRect(r)), tiles_.end());

	std::vector<level_tile> tiles;
	for(std::map<int, tile_map>::const_iterator i = tile_maps_.begin(); i != tile_maps_.end(); ++i) {
		i->second.build_tiles(&tiles, &r);
	}

	foreach(level_tile& t, tiles) {
		add_tile_solid(t);
		tiles_.push_back(t);
		layers_.insert(t.zorder);
	}

	if(std::adjacent_find(tiles_.rbegin(), tiles_.rend(), level_tile_zorder_pos_comparer()) != tiles_.rend()) {
		std::sort(tiles_.begin(), tiles_.end(), level_tile_zorder_pos_comparer());
	}
	prepare_tiles_for_drawing();
}

std::string level::package() const
{
	std::string::const_iterator i = std::find(id_.begin(), id_.end(), '/');
	if(i == id_.end()) {
		return "";
	}

	return std::string(id_.begin(), i);
}

variant level::write() const
{
	std::sort(tiles_.begin(), tiles_.end(), level_tile_zorder_pos_comparer());
	game_logic::wml_formula_callable_serialization_scope serialization_scope;

	variant_builder res;
	res.add("id", id_);
	res.add("version", preferences::version());
	res.add("title", title_);
	res.add("music", music_);
	res.add("segment_width", segment_width_);
	res.add("segment_height", segment_height_);

	if(x_resolution_ || y_resolution_) {
		res.add("x_resolution", x_resolution_);
		res.add("y_resolution", y_resolution_);
	}

	if(gui_algo_str_ != "default") {
		res.add("gui", gui_algo_str_);
	}

	if(dark_) {
		res.add("dark", true);
	}

	if(dark_color_.to_string() != default_dark_color().to_string()) {
		res.add("dark_color", dark_color_.write());
	}

	if(cycle_) {
		res.add("cycle", cycle_);
	}

	if(!sub_level_str_.empty()) {
		res.add("sub_levels", sub_level_str_);
	}

	res.add("dimensions", boundaries().write());

	res.add("xscale", xscale_);
	res.add("yscale", yscale_);
	res.add("auto_move_camera", auto_move_camera_.write());
	res.add("air_resistance", air_resistance_);
	res.add("water_resistance", water_resistance_);

	res.add("preloads", util::join(preloads_));

	if(lock_screen_) {
		res.add("lock_screen", lock_screen_->to_string());
	}

	if(water_) {
		res.add("water", water_->write());
	}

	if(camera_rotation_) {
		res.add("camera_rotation", camera_rotation_->str());
	}

	foreach(const solid_rect& r, solid_rects_) {
		variant_builder node;
		node.add("rect", r.r.write());
		node.add("friction", r.friction);
		node.add("traction", r.traction);
		node.add("damage", r.damage);

		res.add("solid_rect", node.build());
	}

	for(std::map<int, tile_map>::const_iterator i = tile_maps_.begin(); i != tile_maps_.end(); ++i) {
		variant node = i->second.write();
		if(preferences::compiling_tiles) {
			node.add_attr(variant("tiles"), variant(""));
			node.add_attr(variant("unique_tiles"), variant(""));
		}
		res.add("tile_map", node);
	}

	if(preferences::compiling_tiles && !tiles_.empty()) {

		level_object::set_current_palette(palettes_used_);

		int num_tiles = 0;
		int last_zorder = INT_MIN;
		int basex = 0, basey = 0;
		int last_x = 0, last_y = 0;
		std::string tiles_str;
		for(int n = 0; n <= tiles_.size(); ++n) {
			if(n != tiles_.size() && tiles_[n].draw_disabled && tiles_[n].object->has_solid() == false) {
				continue;
			}

			if(n == tiles_.size() || tiles_[n].zorder != last_zorder) {
				if(!tiles_str.empty()) {
					variant_builder node;
					node.add("zorder", last_zorder);
					node.add("x", basex);
					node.add("y", basey);
					node.add("tiles", tiles_str);
					res.add("compiled_tiles", node.build());
				}

				if(n == tiles_.size()) {
					break;
				}

				tiles_str.clear();

				last_zorder = tiles_[n].zorder;

				basex = basey = INT_MAX;
				for(int m = n; m != tiles_.size() && tiles_[m].zorder == tiles_[n].zorder; ++m) {
					if(tiles_[m].x < basex) {
						basex = tiles_[m].x;
					}

					if(tiles_[m].y < basey) {
						basey = tiles_[m].y;
					}
				}

				last_x = basex;
				last_y = basey;
			}

			while(last_y < tiles_[n].y) {
				tiles_str += "\n";
				last_y += TileSize;
				last_x = basex;
			}

			while(last_x < tiles_[n].x) {
				tiles_str += ",";
				last_x += TileSize;
			}

			ASSERT_EQ(last_x, tiles_[n].x);
			ASSERT_EQ(last_y, tiles_[n].y);

			if(tiles_[n].face_right) {
				tiles_str += "~";
			}

			const int xpos = tiles_[n].x;
			const int ypos = tiles_[n].y;
			const int zpos = tiles_[n].zorder;
			const int start_n = n;

			while(n != tiles_.size() && tiles_[n].x == xpos && tiles_[n].y == ypos && tiles_[n].zorder == zpos) {
				char buf[4];
				tiles_[n].object->write_compiled_index(buf);
				if(n != start_n) {
					tiles_str += "|";
				}
				tiles_str += buf;
				++n;
				++num_tiles;
			}

			--n;

			tiles_str += ",";

			last_x += TileSize;
		}

		res.add("num_compiled_tiles", num_tiles);

		//calculate rectangular opaque areas of tiles that allow us
		//to avoid drawing the background. Start by calculating the set
		//of tiles that are opaque.
		typedef std::pair<int,int> OpaqueLoc;
		std::set<OpaqueLoc> opaque;
		foreach(const level_tile& t, tiles_) {
			if(t.object->is_opaque() == false) {
				continue;
			}

			std::map<int, tile_map>::const_iterator tile_itor = tile_maps_.find(t.zorder);
			ASSERT_LOG(tile_itor != tile_maps_.end(), "COULD NOT FIND TILE LAYER IN MAP");
			if(tile_itor->second.x_speed() != 100 || tile_itor->second.y_speed() != 100) {
				//we only consider the layer that moves at 100% speed,
				//since calculating obscured areas at other layers is too
				//complicated.
				continue;
			}

			opaque.insert(std::pair<int,int>(t.x,t.y));
		}

		std::cerr << "BUILDING RECTS...\n";

		std::vector<rect> opaque_rects;

		//keep iterating, finding the largest rectangle we can make of
		//available opaque locations, then removing all those opaque
		//locations from our set, until we have all areas covered.
		while(!opaque.empty()) {
			rect largest_rect;

			//iterate over every opaque location, treating each one
			//as a possible upper-left corner of our rectangle.
			for(std::set<OpaqueLoc>::const_iterator loc_itor = opaque.begin();
			    loc_itor != opaque.end(); ++loc_itor) {
				const OpaqueLoc& loc = *loc_itor;

				std::vector<OpaqueLoc> v;
				v.push_back(loc);

				std::set<OpaqueLoc>::const_iterator find_itor = opaque.end();

				int prev_rows = 0;

				//try to build a top row of a rectangle. After adding each
				//cell, we will try to expand the rectangle downwards, as
				//far as it will go.
				while((find_itor = opaque.find(OpaqueLoc(v.back().first + TileSize, v.back().second))) != opaque.end()) {
					v.push_back(OpaqueLoc(v.back().first + TileSize, v.back().second));

					int rows = 1;

					bool found_non_opaque = false;
					while(found_non_opaque == false) {
						for(int n = rows < prev_rows ? v.size()-1 : 0; n != v.size(); ++n) {
							if(!opaque.count(OpaqueLoc(v[n].first, v[n].second + rows*TileSize))) {
								found_non_opaque = true;
								break;
							}
						}

						if(found_non_opaque == false) {
							++rows;
						}
					}

					prev_rows = rows;

					rect r(v.front().first, v.front().second, v.size()*TileSize, rows*TileSize);
					if(r.w()*r.h() > largest_rect.w()*largest_rect.h()) {
						largest_rect = r;
					}
				} //end while expand rectangle to the right.
			} //end for iterating over all possible rectangle upper-left positions

			std::cerr << "LARGEST_RECT: " << largest_rect.w() << " x " << largest_rect.h() << "\n";

			//have a minimum size for rectangles. If we fail to reach
			//the minimum size then just stop. It's not worth bothering 
			//with lots of small little rectangles.
			if(largest_rect.w()*largest_rect.h() < TileSize*TileSize*32) {
				break;
			}

			opaque_rects.push_back(largest_rect);

			for(std::set<OpaqueLoc>::iterator i = opaque.begin();
			    i != opaque.end(); ) {
				if(i->first >= largest_rect.x() && i->second >= largest_rect.y() && i->first < largest_rect.x2() && i->second < largest_rect.y2()) {
					opaque.erase(i++);
				} else {
					++i;
				}
			}
		} //end searching for rectangles to add.
		std::cerr << "DONE BUILDING RECTS...\n";

		if(!opaque_rects.empty()) {
			std::ostringstream opaque_rects_str;
			foreach(const rect& r, opaque_rects) {
				opaque_rects_str << r.to_string() << ":";
			}

			res.add("opaque_rects", opaque_rects_str.str());

			std::cerr << "RECTS: " << id_ << ": " << opaque_rects.size() << "\n";
		}
	} //end if preferences::compiling

	foreach(entity_ptr ch, chars_) {
		if(!ch->serializable()) {
			continue;
		}

		variant node(ch->write());
		game_logic::wml_formula_callable_serialization_scope::register_serialized_object(ch, node);
		res.add("character", node);
	}

	foreach(const portal& p, portals_) {
		variant_builder node;
		node.add("rect", p.area.write());
		node.add("level", p.level_dest);
		node.add("dest_starting_pos", p.dest_starting_pos);
		node.add("dest", p.dest.to_string());
		node.add("automatic", p.automatic);
		node.add("transition", p.transition);
		res.add("portal", node.build());
	}

	if(right_portal_.level_dest.empty() == false) {
		res.add("next_level", right_portal_.level_dest);
	}

	std::cerr << "PREVIOUS LEVEL: " << left_portal_.level_dest << "\n";
	if(left_portal_.level_dest.empty() == false) {
		res.add("previous_level", left_portal_.level_dest);
	}

	if(background_) {
		if(background_->id().empty()) {
			res.add("background", background_->write());
		} else {
			res.add("background", background_->id());
			res.add("background_offset", background_offset_.write());
		}
	}

	for(std::map<std::string, movement_script>::const_iterator i = movement_scripts_.begin(); i != movement_scripts_.end(); ++i) {
		res.add("script", i->second.write());
	}

	if(num_compiled_tiles_ > 0) {
		res.add("num_compiled_tiles", num_compiled_tiles_);
		foreach(variant compiled_node, wml_compiled_tiles_) {
			res.add("compiled_tiles", compiled_node);
		}
	}

	if(palettes_used_) {
		std::vector<std::string> out;
		unsigned int p = palettes_used_;
		int id = 0;
		while(p) {
			if(p&1) {
				out.push_back(graphics::get_palette_name(id));
			}

			p >>= 1;
			++id;
		}

		res.add("palettes", util::join(out));
	}

	if(background_palette_ != -1) {
		res.add("background_palette", graphics::get_palette_name(background_palette_));
	}

	res.add("vars", vars_);

	variant result = res.build();
	result.add_attr(variant("serialized_objects"), serialization_scope.write_objects(result));
	return result;
}

point level::get_dest_from_str(const std::string& key) const
{
	int ypos = 0;
	if(player()) {
		ypos = player()->get_entity().y();
	}
	if(key == "left") {
		return point(boundaries().x() + 32, ypos);
	} else if(key == "right") {
		return point(boundaries().x2() - 128, ypos);
	} else {
		return point();
	}
}

const std::string& level::previous_level() const
{
	return left_portal_.level_dest;
}

const std::string& level::next_level() const
{
	return right_portal_.level_dest;
}

void level::set_previous_level(const std::string& name)
{
	left_portal_.level_dest = name;
	left_portal_.dest_str = "right";
	left_portal_.dest_starting_pos = false;
	left_portal_.automatic = true;
}

void level::set_next_level(const std::string& name)
{
	right_portal_.level_dest = name;
	right_portal_.dest_str = "left";
	right_portal_.dest_starting_pos = false;
	right_portal_.automatic = true;
}

namespace {
//counter incremented every time the level is drawn.
int draw_count = 0;
}

void level::draw_layer(int layer, int x, int y, int w, int h) const
{
	if(layer >= 1000 && editor_ && show_foreground_ == false) {
		return;
	}

	for(std::map<std::string, sub_level_data>::const_iterator i = sub_levels_.begin(); i != sub_levels_.end(); ++i) {
		if(i->second.active) {
			glPushMatrix();
			glTranslatef(i->second.xoffset, i->second.yoffset, 0.0);
			i->second.lvl->draw_layer(layer, x - i->second.xoffset, y - i->second.yoffset - TileSize, w, h + TileSize);
			glPopMatrix();
		}
	}

	if(editor_ && layer == highlight_layer_) {
		const GLfloat alpha = 0.3 + (1.0+sin(draw_count/5.0))*0.35;
		glColor4f(1.0, 1.0, 1.0, alpha);

	} else if(editor_ && hidden_layers_.count(layer)) {
		glColor4f(1.0, 1.0, 1.0, 0.3);
	}

	glPushMatrix();

	graphics::distortion_translation distort_translation;
	
	// parallax scrolling for tiles.
	std::map<int, tile_map>::const_iterator tile_map_iterator = tile_maps_.find(layer);
	if(tile_map_iterator != tile_maps_.end()) {
		int scrollx = tile_map_iterator->second.x_speed();
		int scrolly = tile_map_iterator->second.y_speed();

		const int diffx = ((scrollx - 100)*x)/100;
		const int diffy = ((scrolly - 100)*y)/100;

		glTranslatef(diffx, diffy, 0.0);
		distort_translation.translate(diffx, diffy);
		
		//here, we adjust the screen bounds (they're a first order optimization) to account for the parallax shift
		x -= diffx;
		y -= diffy;
	} 

	typedef std::vector<level_tile>::const_iterator itor;
	std::pair<itor,itor> range = std::equal_range(tiles_.begin(), tiles_.end(), layer, level_tile_zorder_comparer());

	itor tile_itor = std::lower_bound(range.first, range.second, y,
	                          level_tile_y_pos_comparer());

	if(tile_itor == range.second) {
		glPopMatrix();
		return;
	}

	std::map<int, layer_blit_info>::iterator layer_itor = blit_cache_.find(layer);
	if(layer_itor == blit_cache_.end()) {
		glPopMatrix();
		return;
	}

	const level_tile* t = &*tile_itor;
	const level_tile* end_tiles = &*tiles_.begin() + tiles_.size();

	layer_blit_info& blit_info = layer_itor->second;

	const rect tile_positions(x/32 - (x < 0 ? 1 : 0), y/32 - (y < 0 ? 1 : 0),
	                          (x + w)/32 - (x + w < 0 ? 1 : 0),
							  (y + h)/32 - (y + h < 0 ? 1 : 0));

	std::vector<layer_blit_info::IndexType>& opaque_indexes = blit_info.opaque_indexes;
	std::vector<layer_blit_info::IndexType>& translucent_indexes = blit_info.translucent_indexes;

	if(blit_info.tile_positions != tile_positions || editor_) {
		blit_info.tile_positions = tile_positions;

		opaque_indexes.clear();
		translucent_indexes.clear();

		int ystart = std::max<int>(0, (y - blit_info.ybase)/TileSize);
		int yend = std::min<int>(blit_info.indexes.size(), (y + h - blit_info.ybase)/TileSize + 1);

		for(; ystart < yend; ++ystart) {
			const std::vector<layer_blit_info::IndexType>& indexes = blit_info.indexes[ystart];
			int xstart = std::max<int>(0, (x - blit_info.xbase)/TileSize);
			int xend = std::min<int>(indexes.size(), (x + w - blit_info.xbase)/TileSize + 1);
			for(; xstart < xend; ++xstart) {
				if(indexes[xstart] != TILE_INDEX_TYPE_MAX) {
					if(indexes[xstart] > 0) {
						GLint index = indexes[xstart];
						opaque_indexes.push_back(index);
						opaque_indexes.push_back(index+1);
						opaque_indexes.push_back(index+2);
						opaque_indexes.push_back(index+1);
						opaque_indexes.push_back(index+2);
						opaque_indexes.push_back(index+3);
						ASSERT_INDEX_INTO_VECTOR(index, blit_info.blit_vertexes);
						ASSERT_INDEX_INTO_VECTOR(index+3, blit_info.blit_vertexes);
					} else {
						GLint index = -indexes[xstart];
						translucent_indexes.push_back(index);
						translucent_indexes.push_back(index+1);
						translucent_indexes.push_back(index+2);
						translucent_indexes.push_back(index+1);
						translucent_indexes.push_back(index+2);
						translucent_indexes.push_back(index+3);
						ASSERT_INDEX_INTO_VECTOR(index, blit_info.blit_vertexes);
						ASSERT_INDEX_INTO_VECTOR(index+3, blit_info.blit_vertexes);
					}
				}
			}
		}
	}

	glDisable(GL_BLEND);
	draw_layer_solid(layer, x, y, w, h);
	if(blit_info.texture_id != GLuint(-1)) {
		graphics::texture::set_current_texture(blit_info.texture_id);
	}

	if(!opaque_indexes.empty()) {
		glVertexPointer(2, GL_SHORT, sizeof(tile_corner), &blit_info.blit_vertexes[0].vertex[0]);
		glTexCoordPointer(2, GL_FLOAT, sizeof(tile_corner), &blit_info.blit_vertexes[0].uv[0]);
		glDrawElements(GL_TRIANGLES, opaque_indexes.size(), TILE_INDEX_TYPE, &opaque_indexes[0]);
	}
	glEnable(GL_BLEND);

	if(!translucent_indexes.empty()) {
		glVertexPointer(2, GL_SHORT, sizeof(tile_corner), &blit_info.blit_vertexes[0].vertex[0]);
		glTexCoordPointer(2, GL_FLOAT, sizeof(tile_corner), &blit_info.blit_vertexes[0].uv[0]);

		if(blit_info.texture_id == GLuint(-1)) {
			//we have multiple different texture ID's in this layer. This means
			//we will draw each tile seperately.
			for(int n = 0; n < translucent_indexes.size(); n += 6) {
				graphics::texture::set_current_texture(blit_info.vertex_texture_ids[translucent_indexes[n]/4]);
				glDrawElements(GL_TRIANGLES, 6, TILE_INDEX_TYPE, &translucent_indexes[n]);
			}
		} else {
			//we have just one texture ID and so can draw all tiles in one call.
			glDrawElements(GL_TRIANGLES, translucent_indexes.size(), TILE_INDEX_TYPE, &translucent_indexes[0]);
		}
	}

	glPopMatrix();

	glColor4f(1.0, 1.0, 1.0, 1.0);
}

void level::draw_layer_solid(int layer, int x, int y, int w, int h) const
{
	solid_color_rect arg;
	arg.layer = layer;
	typedef std::vector<solid_color_rect>::const_iterator SolidItor;
	std::pair<SolidItor, SolidItor> solid = std::equal_range(solid_color_rects_.begin(), solid_color_rects_.end(), arg, solid_color_rect_cmp());
	if(solid.first != solid.second) {
		const rect viewport(x, y, w, h);

		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		while(solid.first != solid.second) {
			rect area = solid.first->area;
			if(!rects_intersect(area, viewport)) {
				++solid.first;
				continue;
			}

			area = intersection_rect(area, viewport);

			solid.first->color.set_as_current_color();
			GLshort varray[] = {
			  area.x(), area.y(),
			  area.x() + area.w(), area.y(),
			  area.x(), area.y() + area.h(),
			  area.x() + area.w(), area.y() + area.h(),
			};

			glVertexPointer(2, GL_SHORT, 0, varray);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			++solid.first;
		}
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glColor4ub(255, 255, 255, 255);
	}
}

void level::prepare_tiles_for_drawing()
{
	level_object::set_current_palette(palettes_used_);

	solid_color_rects_.clear();
	blit_cache_.clear();

	for(int n = 0; n != tiles_.size(); ++n) {
		if(!is_arcade_level() && tiles_[n].object->solid_color()) {
			continue;
		}

		//in the editor we want to draw the whole level, so don't exclude
		//things outside the level bounds.
		if(!editor_ && (tiles_[n].x <= boundaries().x() - TileSize || tiles_[n].y <= boundaries().y() - TileSize || tiles_[n].x >= boundaries().x2() || tiles_[n].y >= boundaries().y2())) {
			continue;
		}

		layer_blit_info& blit_info = blit_cache_[tiles_[n].zorder];
		if(blit_info.xbase == -1) {
			blit_info.texture_id = tiles_[n].object->texture().get_id();
			blit_info.xbase = tiles_[n].x;
			blit_info.ybase = tiles_[n].y;
		}

		if(tiles_[n].x < blit_info.xbase) {
			blit_info.xbase = tiles_[n].x;
		}

		if(tiles_[n].y < blit_info.ybase) {
			blit_info.ybase = tiles_[n].y;
		}
	}

	for(int n = 0; n != tiles_.size(); ++n) {
		if(!editor_ && (tiles_[n].x <= boundaries().x() - TileSize || tiles_[n].y <= boundaries().y() - TileSize || tiles_[n].x >= boundaries().x2() || tiles_[n].y >= boundaries().y2())) {
			continue;
		}

		if(!is_arcade_level() && tiles_[n].object->solid_color()) {
			tiles_[n].draw_disabled = true;
			if(!solid_color_rects_.empty()) {
				solid_color_rect& r = solid_color_rects_.back();
				if(r.layer == tiles_[n].zorder && r.color.rgba() == tiles_[n].object->solid_color()->rgba() && r.area.y() == tiles_[n].y && r.area.x() + r.area.w() == tiles_[n].x) {
					r.area = rect(r.area.x(), r.area.y(), r.area.w() + TileSize, r.area.h());
					continue;
				}
			}
				
			solid_color_rect r;
			r.color = *tiles_[n].object->solid_color();
			r.area = rect(tiles_[n].x, tiles_[n].y, TileSize, TileSize);
			r.layer = tiles_[n].zorder;
			solid_color_rects_.push_back(r);
			continue;
		}

		layer_blit_info& blit_info = blit_cache_[tiles_[n].zorder];

		tiles_[n].draw_disabled = false;

		blit_info.blit_vertexes.resize(blit_info.blit_vertexes.size() + 4);
		const int npoints = level_object::calculate_tile_corners(&blit_info.blit_vertexes[blit_info.blit_vertexes.size() - 4], tiles_[n]);
		if(npoints == 0) {
			blit_info.blit_vertexes.resize(blit_info.blit_vertexes.size() - 4);
		} else {
			blit_info.vertex_texture_ids.push_back(tiles_[n].object->texture().get_id());
			if(blit_info.vertex_texture_ids.back() != blit_info.texture_id) {
				blit_info.texture_id = GLuint(-1);
			}

			const int xtile = (tiles_[n].x - blit_info.xbase)/TileSize;
			const int ytile = (tiles_[n].y - blit_info.ybase)/TileSize;
			ASSERT_GE(xtile, 0);
			ASSERT_GE(ytile, 0);
			if(blit_info.indexes.size() <= ytile) {
				blit_info.indexes.resize(ytile+1);
			}

			if(blit_info.indexes[ytile].size() <= xtile) {
				blit_info.indexes[ytile].resize(xtile+1, TILE_INDEX_TYPE_MAX);
			}

			blit_info.indexes[ytile][xtile] = (blit_info.blit_vertexes.size() - 4) * (tiles_[n].object->is_opaque() ? 1 : -1);
		}
	}

	for(int n = 1; n < solid_color_rects_.size(); ++n) {
		solid_color_rect& a = solid_color_rects_[n-1];
		solid_color_rect& b = solid_color_rects_[n];
		if(a.area.x() == b.area.x() && a.area.x2() == b.area.x2() && a.area.y() + a.area.h() == b.area.y() && a.layer == b.layer) {
			a.area = rect(a.area.x(), a.area.y(), a.area.w(), a.area.h() + b.area.h());
			b.area = rect(0,0,0,0);
		}
	}

	solid_color_rects_.erase(std::remove_if(solid_color_rects_.begin(), solid_color_rects_.end(), solid_color_rect_empty()), solid_color_rects_.end());

	//remove tiles that are obscured by other tiles.
	std::set<std::pair<int, int> > opaque;
	for(int n = tiles_.size(); n > 0; --n) {
		level_tile& t = tiles_[n-1];
		const tile_map& map = tile_maps_[t.zorder];
		if(map.x_speed() != 100 || map.y_speed() != 100) {
			while(n != 0 && tiles_[n-1].zorder == t.zorder) {
				--n;
			}

			continue;
		}

		if(!t.draw_disabled && opaque.count(std::pair<int,int>(t.x, t.y))) {
			t.draw_disabled = true;
			continue;
		}

		if(t.object->is_opaque()) {
			opaque.insert(std::pair<int,int>(t.x, t.y));
		}
	}

}

namespace {
bool sort_entity_drawing_pos(const entity_ptr& a, const entity_ptr& b) {
	//the reverse_global_vertical_zordering flag is set in the player object (our general repository for all major game rules et al).  It's meant to reverse vertical sorting of objects in the same zorder, depending on whether objects are being viewed from above, or below.  In frogatto proper, objects at a higher vertical position should overlap those below.  In a top-down game, the reverse is desirable.
	if(level::current().player() && level::current().player()->reverse_global_vertical_zordering()){
			return a->zorder() < b->zorder() ||
			a->zorder() == b->zorder() && a->zsub_order() < b->zsub_order() ||
			a->zorder() == b->zorder() && a->zsub_order() == b->zsub_order() && a->midpoint().y < b->midpoint().y ||
			a->zorder() == b->zorder() && a->zsub_order() == b->zsub_order() && a->midpoint().y == b->midpoint().y && a->midpoint().x < b->midpoint().x;
			
		}else{
		return a->zorder() < b->zorder() ||
				a->zorder() == b->zorder() && a->zsub_order() < b->zsub_order() ||
				a->zorder() == b->zorder() && a->zsub_order() == b->zsub_order() && a->midpoint().y > b->midpoint().y ||
				a->zorder() == b->zorder() && a->zsub_order() == b->zsub_order() && a->midpoint().y == b->midpoint().y && a->midpoint().x > b->midpoint().x;
		}
	}
}

void level::draw_status() const
{
	if(gui_algorithm_) {
		gui_algorithm_->draw(*this);
		iphone_controls::draw();
	}

	if(current_speech_dialog()) {
		current_speech_dialog()->draw();
	}
}

namespace {
void draw_entity(const entity& obj, int x, int y, bool editor) {
	const std::pair<int,int>* scroll_speed = obj.parallax_scale_millis();

	if(scroll_speed) {
		glPushMatrix();
		const int scrollx = scroll_speed->first;
		const int scrolly = scroll_speed->second;

		const int diffx = ((scrollx - 1000)*x)/1000;
		const int diffy = ((scrolly - 1000)*y)/1000;

		glTranslatef(diffx, diffy, 0.0);
	}

	obj.draw();
	if(editor) {
		obj.draw_group();
	}

	if(scroll_speed) {
		glPopMatrix();
	}
}
}

extern std::vector<rect> background_rects_drawn;

void level::draw(int x, int y, int w, int h) const
{
	sound::process();

	++draw_count;

	const int start_x = x;
	const int start_y = y;
	const int start_w = w;
	const int start_h = h;

	const int ticks = SDL_GetTicks();
	x -= widest_tile_;
	y -= highest_tile_;
	w += widest_tile_;
	h += highest_tile_;

	const std::vector<entity_ptr>* chars_ptr = &active_chars_;
	std::vector<entity_ptr> editor_chars_buf;
	
	if(editor_) {
		rect screen_area(x, y, w, h);

		//in the editor draw all characters that are on screen as well
		//as active ones.
		foreach(const entity_ptr& c, chars_) {
			if(std::find(active_chars_.begin(), active_chars_.end(), c) != active_chars_.end() || rects_intersect(c->draw_rect(), screen_area)) {
				editor_chars_buf.push_back(c);
			}
		}
		std::sort(editor_chars_buf.begin(), editor_chars_buf.end(), sort_entity_drawing_pos);
		chars_ptr = &editor_chars_buf;
	}
	const std::vector<entity_ptr>& chars = *chars_ptr;

	std::vector<entity_ptr>::const_iterator entity_itor = chars.begin();

	
	/*std::cerr << "SUMMARY " << cycle_ << ": ";
	foreach(const entity_ptr& e, chars_) {
		std::cerr << e->debug_description() << "(" << e->zsub_order() << "):";
	}
	
	std::cerr << "\n";*/
	
	bool water_drawn = true;
	int water_zorder = 0;
	if(water_) {
		water_drawn = false;
		water_zorder = water_->zorder();
	}

	std::set<int>::const_iterator layer = layers_.begin();

	for(; layer != layers_.end(); ++layer) {
		if(!water_drawn && *layer > water_zorder) {
			water_->draw(x, y, w, h);
			water_drawn = true;
		}

		while(entity_itor != chars.end() && (*entity_itor)->zorder() <= *layer) {
			draw_entity(**entity_itor, x, y, editor_);
			++entity_itor;
		}

		draw_layer(*layer, x, y, w, h);
	}

	if(!water_drawn) {
		water_->draw(x, y, w, h);
			water_drawn = true;
	}

	while(entity_itor != chars.end()) {
		draw_entity(**entity_itor, x, y, editor_);
		++entity_itor;
	}

	if(editor_) {
		foreach(const entity_ptr& obj, chars_) {
			if(entity_collides_with_level(*this, *obj, MOVE_NONE)) {
				//if the entity is colliding with the level, then draw
				//it in red to mark as 'bad'.
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				const GLfloat alpha = 0.5 + sin(draw_count/5.0)*0.5;
				glColor4f(1.0, 0.0, 0.0, alpha);
				obj->draw();
				glColor4f(1.0, 1.0, 1.0, 1.0);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
		}
	}

	if(editor_highlight_ || !editor_selection_.empty()) {
		if(editor_highlight_) {
			draw_entity(*editor_highlight_, x, y, true);
		}

		foreach(const entity_ptr& e, editor_selection_) {
			draw_entity(*e, x, y, true);
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		const GLfloat alpha = 0.5 + sin(draw_count/5.0)*0.5;
		glColor4f(1.0, 1.0, 1.0, alpha);

		if(editor_highlight_) {
			draw_entity(*editor_highlight_, x, y, true);
		}

		foreach(const entity_ptr& e, editor_selection_) {
			draw_entity(*e, x, y, true);
		}

		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	draw_debug_solid(x, y, w, h);

	if(background_) {
		background_->draw_foreground(start_x, start_y, 0.0, cycle());
	}

	calculate_lighting(start_x, start_y, start_w, start_h);
}

void level::calculate_lighting(int x, int y, int w, int h) const
{
	if(!dark_ || editor_ || texture_frame_buffer::unsupported()) {
		return;
	}

	//find all the lights in the level
	static std::vector<const light*> lights;
	lights.clear();
	foreach(const entity_ptr& c, active_chars_) {
		foreach(const light_ptr& lt, c->lights()) {
			lights.push_back(lt.get());
		}
	}

	{
		glBlendFunc(GL_ONE, GL_ONE);
		rect screen_area(x, y, w, h);
		const texture_frame_buffer::render_scope scope;

		glClearColor(dark_color_.r()/255.0, dark_color_.g()/255.0, dark_color_.b()/255.0, dark_color_.a()/255.0);
		glClear(GL_COLOR_BUFFER_BIT);
		const unsigned char color[] = { dark_color_.r(), dark_color_.g(), dark_color_.b(), dark_color_.a() };
		foreach(const light* lt, lights) {
			lt->draw(screen_area, color);
		}
	}

	//now blit the light buffer onto the screen
	texture_frame_buffer::set_as_current_texture();

	const GLfloat tcarray[] = { 0, 0, 0, 1, 1, 0, 1, 1 };
	const GLfloat tcarray_rotated[] = { 0, 1, 1, 1, 0, 0, 1, 0 };
	GLfloat varray[] = { x, y + h, x, y, x + w, y + h, x + w, y };
	glVertexPointer(2, GL_FLOAT, 0, varray);
	glTexCoordPointer(2, GL_FLOAT, 0,
	               preferences::screen_rotated() ? tcarray_rotated : tcarray);
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void level::draw_debug_solid(int x, int y, int w, int h) const
{
	if(preferences::show_debug_hitboxes() == false) {
		return;
	}

	const int tile_x = x/TileSize - 2;
	const int tile_y = y/TileSize - 2;

	for(int xpos = 0; xpos < w/TileSize + 4; ++xpos) {
		for(int ypos = 0; ypos < h/TileSize + 4; ++ypos) {
			const tile_pos pos(tile_x + xpos, tile_y + ypos);
			const tile_solid_info* info = solid_.find(pos);
			if(info == NULL) {
				continue;
			}

			const int xpixel = (tile_x + xpos)*TileSize;
			const int ypixel = (tile_y + ypos)*TileSize;

			if(info->all_solid) {
				graphics::draw_rect(rect(xpixel, ypixel, TileSize, TileSize), info->info.damage ? graphics::color(255, 0, 0, 196) : graphics::color(255, 255, 255, 196));
			} else {
				std::vector<GLshort> v;
				glDisable(GL_TEXTURE_2D);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				for(int suby = 0; suby != TileSize; ++suby) {
					for(int subx = 0; subx != TileSize; ++subx) {
						if(info->bitmap.test(suby*TileSize + subx)) {
							v.push_back(xpixel + subx + 1);
							v.push_back(ypixel + suby + 1);
						}
					}
				}

				if(!v.empty()) {
					if(info->info.damage) {
						glColor4ub(255, 0, 0, 196);
					} else {
						glColor4ub(255, 255, 255, 196);
					}

					glPointSize(1);
					glVertexPointer(2, GL_SHORT, 0, &v[0]);
					glDrawArrays(GL_POINTS, 0, v.size()/2);
				}
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glEnable(GL_TEXTURE_2D);
			}
		}
	}

	glColor4ub(255, 255, 255, 255);
}

void level::draw_background(int x, int y, int rotation) const
{
	if(show_background_ == false) {
		return;
	}

	if(water_) {
		water_->begin_drawing();
	}

	foreach(const entity_ptr& c, active_chars_) {
		c->setup_drawing();
	}

	if(background_) {
		static std::vector<rect> opaque_areas;
		opaque_areas.clear();
		int screen_width = graphics::screen_width();
		int screen_height = graphics::screen_height();
		if(zoom_level_ < 1.0) {
			screen_width /= zoom_level_.as_float();
			screen_height /= zoom_level_.as_float();
		}

		rect screen_area(x, y, screen_width, screen_height);
		foreach(const rect& r, opaque_rects_) {
			if(rects_intersect(r, screen_area)) {

				rect intersection = intersection_rect(r, screen_area);

				if(intersection.w() == screen_area.w() || intersection.h() == screen_area.h()) {
					rect result[2];
					const int nrects = rect_difference(screen_area, intersection, result);
					ASSERT_LOG(nrects <= 2, "TOO MANY RESULTS " << nrects << " IN " << screen_area << " - " << intersection);
					if(nrects < 1) {
						//background is completely obscured, so return
						return;
					} else if(nrects == 1) {
						screen_area = result[0];
					} else {
						opaque_areas.push_back(intersection);
					}
				} else if(intersection.w()*intersection.h() >= TileSize*TileSize*8) {
					opaque_areas.push_back(intersection);
				}
			}
		}

		background_->draw(x, y, screen_area, opaque_areas, rotation, cycle());
	} else {
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}

void level::process()
{
	if(gui_algorithm_) {
		gui_algorithm_->process(*this);
	}

	const int LevelPreloadFrequency = 500; //10 seconds
	//see if we have levels to pre-load. Load one periodically.
	if((cycle_%LevelPreloadFrequency) == 0) {
		const int index = cycle_/LevelPreloadFrequency;
		if(index < preloads_.size()) {
			preload_level(preloads_[index]);
		}
	}

	controls::read_local_controls();

	multiplayer::send_and_receive();

	do_processing();

	if(speech_dialogs_.empty() == false) {
		if(speech_dialogs_.top()->process()) {
			speech_dialogs_.pop();
		}
	}
}

void level::process_draw()
{
	foreach(const entity_ptr& e, active_chars_) {
		e->handle_event(OBJECT_EVENT_DRAW);
	}
}

namespace {
bool compare_entity_num_parents(const entity_ptr& a, const entity_ptr& b) {
	const int deptha = a->parent_depth();
	const int depthb = b->parent_depth();
	return deptha < depthb || deptha == depthb && a->standing_on().get() < b->standing_on().get() || deptha == depthb && (a->standing_on().get() ? true : false) == (b->standing_on().get() ? true : false) && a->is_human() < b->is_human();
}
}

void level::set_active_chars()
{
	const int screen_left = last_draw_position().x/100;
	const int screen_right = last_draw_position().x/100 + graphics::screen_width();
	const int screen_top = last_draw_position().y/100;
	const int screen_bottom = last_draw_position().y/100 + graphics::screen_height();

	const rect screen_area(screen_left, screen_top, screen_right - screen_left, screen_bottom - screen_top);
	active_chars_.clear();
	foreach(entity_ptr& c, chars_) {
		const bool is_active = c->is_active(screen_area);

		if(is_active) {
			if(c->group() >= 0) {
				assert(c->group() < groups_.size());
				const entity_group& group = groups_[c->group()];
				active_chars_.insert(active_chars_.end(), group.begin(), group.end());
			} else {
				active_chars_.push_back(c);
			}
		} else { //char is inactive
			if( c->dies_on_inactive() ){
				if(c->label().empty() == false) {
					c->die_with_no_event();
					chars_by_label_.erase(c->label());
				}
				
				c = entity_ptr(); //can't delete it while iterating over the container, so we null it for later removal
			}
		}
	}
	
	chars_.erase(std::remove(chars_.begin(), chars_.end(), entity_ptr()), chars_.end());

	std::sort(active_chars_.begin(), active_chars_.end());
	active_chars_.erase(std::unique(active_chars_.begin(), active_chars_.end()), active_chars_.end());
	std::sort(active_chars_.begin(), active_chars_.end(), sort_entity_drawing_pos);
}

void level::do_processing()
{
	if(cycle_ == 0) {
		const std::vector<entity_ptr> chars = chars_;
		foreach(const entity_ptr& e, chars) {
			e->handle_event(OBJECT_EVENT_START_LEVEL);
		}
	}

	++cycle_;
	if(!player_) {
		return;
	}

	const int ticks = SDL_GetTicks();
	set_active_chars();
	detect_user_collisions(*this);

	
/*
	std::cerr << "SUMMARY " << cycle_ << ": ";
	foreach(const entity_ptr& e, chars_) {
		std::cerr << e->debug_description() << "(" << (e->is_human() ? "HUMAN," : "") << e->centi_x() << "," << e->centi_y() << "):";
	}

	std::cerr << "\n";
	*/

	int checksum = 0;
	foreach(const entity_ptr& e, chars_) {
		checksum += e->x() + e->y();
	}

	controls::set_checksum(cycle_, checksum);

	const int ActivationDistance = 700;

	std::vector<entity_ptr> active_chars = active_chars_;
	std::sort(active_chars.begin(), active_chars.end(), compare_entity_num_parents);
	if(time_freeze_ >= 1000) {
		time_freeze_ -= 1000;
		active_chars = chars_immune_from_time_freeze_;
	}
	foreach(const entity_ptr& c, active_chars) {
		if(!c->destroyed() || c->is_human()) {
			c->process(*this);
		}

		if(c->destroyed() && !c->is_human()) {
			if(player_ && !c->respawn() && c->get_id() != -1) {
				player_->is_human()->object_destroyed(id(), c->get_id());
			}

			erase_char(c);
		}
	}

	if(water_) {
		water_->process(*this);
	}

	solid_chars_.clear();
}

void level::erase_char(entity_ptr c)
{

	if(c->label().empty() == false) {
		chars_by_label_.erase(c->label());
	}
	chars_.erase(std::remove(chars_.begin(), chars_.end(), c), chars_.end());
	if(c->group() >= 0) {
		assert(c->group() < groups_.size());
		entity_group& group = groups_[c->group()];
		group.erase(std::remove(group.begin(), group.end(), c), group.end());
	}

	solid_chars_.clear();
}

bool level::is_solid(const level_solid_map& map, const entity& e, const std::vector<point>& points, const surface_info** surf_info) const
{
	const tile_solid_info* info = NULL;
	int prev_x = INT_MIN, prev_y = INT_MIN;

	const frame& current_frame = e.current_frame();
	
	for(std::vector<point>::const_iterator p = points.begin(); p != points.end(); ++p) {
		int x, y;
		if(prev_x != INT_MIN) {
			const int diff_x = (p->x - (p-1)->x) * (e.face_right() ? 1 : -1);
			const int diff_y = p->y - (p-1)->y;

			x = prev_x + diff_x;
			y = prev_y + diff_y;
			
			if(x < 0 || y < 0 || x >= TileSize || y >= TileSize) {
				//we need to recalculate the info, since we've stepped into
				//another tile.
				prev_x = INT_MIN;
			}
		}
		
		if(prev_x == INT_MIN) {
			x = e.x() + (e.face_right() ? p->x : (current_frame.width() - 1 - p->x));
			y = e.y() + p->y;

			tile_pos pos(x/TileSize, y/TileSize);
			x = x%TileSize;
			y = y%TileSize;
			if(x < 0) {
				pos.first--;
				x += 32;
			}

			if(y < 0) {
				pos.second--;
				y += 32;
			}

			info = map.find(pos);
		}

		if(info != NULL) {
			if(info->all_solid) {
				if(surf_info) {
					*surf_info = &info->info;
				}

				return true;
			}
		
			const int index = y*TileSize + x;
			if(info->bitmap.test(index)) {
				if(surf_info) {
					*surf_info = &info->info;
				}

				return true;
			}
		}

		prev_x = x;
		prev_y = y;
	}

	return false;
}

bool level::is_solid(const level_solid_map& map, int x, int y, const surface_info** surf_info) const
{
	tile_pos pos(x/TileSize, y/TileSize);
	x = x%TileSize;
	y = y%TileSize;
	if(x < 0) {
		pos.first--;
		x += 32;
	}

	if(y < 0) {
		pos.second--;
		y += 32;
	}

	const tile_solid_info* info = map.find(pos);
	if(info != NULL) {
		if(info->all_solid) {
			if(surf_info) {
				*surf_info = &info->info;
			}

			return true;
		}
		
		const int index = y*TileSize + x;
		if(info->bitmap.test(index)) {
			if(surf_info) {
				*surf_info = &info->info;
			}

			return true;
		} else {
			return false;
		}
	}

	return false;
}

bool level::standable(const rect& r, const surface_info** info) const
{
	const int ybegin = r.y();
	const int yend = r.y2();
	const int xbegin = r.x();
	const int xend = r.x2();

	for(int y = ybegin; y != yend; ++y) {
		for(int x = xbegin; x != xend; ++x) {
			if(standable(x, y, info)) {
				return true;
			}
		}
	}

	return false;
}

bool level::standable(int x, int y, const surface_info** info) const
{
	if(is_solid(solid_, x, y, info) || is_solid(standable_, x, y, info)) {
	   return true;
	}

	return false;
}

bool level::standable_tile(int x, int y, const surface_info** info) const
{
	if(is_solid(solid_, x, y, info) || is_solid(standable_, x, y, info)) {
		return true;
	}
	
	return false;
}


bool level::solid(int x, int y, const surface_info** info) const
{
	return is_solid(solid_, x, y, info);
}

bool level::solid(const entity& e, const std::vector<point>& points, const surface_info** info) const
{
	return is_solid(solid_, e, points, info);
}

bool level::solid(const rect& r, const surface_info** info) const
{
	//TODO: consider optimizing this function.
	const int ybegin = r.y();
	const int yend = r.y2();
	const int xbegin = r.x();
	const int xend = r.x2();

	for(int y = ybegin; y != yend; ++y) {
		for(int x = xbegin; x != xend; ++x) {
			if(solid(x, y, info)) {
				return true;
			}
		}
	}

	return false;
}

bool level::may_be_solid_in_rect(const rect& r) const
{
	int x = r.x();
	int y = r.y();
	tile_pos pos(x/TileSize, y/TileSize);
	x = x%TileSize;
	y = y%TileSize;
	if(x < 0) {
		pos.first--;
		x += 32;
	}

	if(y < 0) {
		pos.second--;
		y += 32;
	}

	const int x2 = (x + r.w())/TileSize + ((x + r.w())%TileSize ? 1 : 0);
	const int y2 = (y + r.h())/TileSize + ((y + r.h())%TileSize ? 1 : 0);

	for(int ypos = 0; ypos < y2; ++ypos) {
		for(int xpos = 0; xpos < x2; ++xpos) {
			if(solid_.find(tile_pos(pos.first + xpos, pos.second + ypos))) {
				return true;
			}
		}
	}

	return false;
}

void level::set_solid_area(const rect& r, bool solid)
{
	std::string empty_info;
	for(int y = r.y(); y < r.y2(); ++y) {
		for(int x = r.x(); x < r.x2(); ++x) {
			set_solid(solid_, x, y, 0, 0, 0, empty_info, solid);
		}
	}
}

entity_ptr level::board(int x, int y) const
{
	for(std::vector<entity_ptr>::const_iterator i = active_chars_.begin();
	    i != active_chars_.end(); ++i) {
		const entity_ptr& c = *i;
		if(c->boardable_vehicle() && c->point_collides(x, y)) {
			return c;
		}
	}

	return entity_ptr();
}

void level::add_tile(const level_tile& t)
{
	std::vector<level_tile>::iterator itor = std::lower_bound(tiles_.begin(), tiles_.end(), t, level_tile_zorder_comparer());
	tiles_.insert(itor, t);
	add_tile_solid(t);
	layers_.insert(t.zorder);
	prepare_tiles_for_drawing();
}

void level::add_tile_rect(int zorder, int x1, int y1, int x2, int y2, const std::string& str)
{
	add_tile_rect_vector(zorder, x1, y1, x2, y2, std::vector<std::string>(1, str));
}

void level::add_tile_rect_vector(int zorder, int x1, int y1, int x2, int y2, const std::vector<std::string>& tiles)
{
	if(x1 > x2) {
		std::swap(x1, x2);
	}

	if(y1 > y2) {
		std::swap(y1, y2);
	}
	add_tile_rect_vector_internal(zorder, x1, y1, x2, y2, tiles);
}

void level::set_tile_layer_speed(int zorder, int x_speed, int y_speed)
{
	tile_map& m = tile_maps_[zorder];
	m.set_zorder(zorder);
	m.set_speed(x_speed, y_speed);
}

void level::refresh_tile_rect(int x1, int y1, int x2, int y2)
{
	rebuild_tiles_rect(rect(x1-128, y1-128, (x2 - x1) + 256, (y2 - y1) + 256));
}

namespace {
int round_tile_size(int n)
{
	if(n >= 0) {
		return n - n%TileSize;
	} else {
		n = -n + 32;
		return -(n - n%TileSize);
	}
}

}

bool level::add_tile_rect_vector_internal(int zorder, int x1, int y1, int x2, int y2, const std::vector<std::string>& tiles)
{
	if(tiles.empty()) {
		return false;
	}

	if(x1 > x2) {
		std::swap(x1, x2);
	}

	if(y1 > y2) {
		std::swap(y1, y2);
	}

	x1 = round_tile_size(x1);
	y1 = round_tile_size(y1);
	x2 = round_tile_size(x2 + TileSize);
	y2 = round_tile_size(y2 + TileSize);

	tile_map& m = tile_maps_[zorder];
	m.set_zorder(zorder);

	bool changed = false;

	int index = 0;
	for(int x = x1; x < x2; x += 32) {
		for(int y = y1; y < y2; y += 32) {
			changed = m.set_tile(x, y, tiles[index]) || changed;
			if(index+1 < tiles.size()) {
				++index;
			}
		}
	}

	return changed;
}

void level::get_tile_rect(int zorder, int x1, int y1, int x2, int y2, std::vector<std::string>& tiles) const
{
	if(x1 > x2) {
		std::swap(x1, x2);
	}

	if(y1 > y2) {
		std::swap(y1, y2);
	}

	x1 = round_tile_size(x1);
	y1 = round_tile_size(y1);
	x2 = round_tile_size(x2 + TileSize);
	y2 = round_tile_size(y2 + TileSize);

	std::map<int, tile_map>::const_iterator map_iterator = tile_maps_.find(zorder);
	if(map_iterator == tile_maps_.end()) {
		tiles.push_back("");
		return;
	}
	const tile_map& m = map_iterator->second;

	for(int x = x1; x < x2; x += 32) {
		for(int y = y1; y < y2; y += 32) {
			tiles.push_back(m.get_tile_from_pixel_pos(x, y));
		}
	}
}

void level::get_all_tiles_rect(int x1, int y1, int x2, int y2, std::map<int, std::vector<std::string> >& tiles) const
{
	for(std::set<int>::const_iterator i = layers_.begin(); i != layers_.end(); ++i) {
		if(hidden_layers_.count(*i)) {
			continue;
		}

		std::vector<std::string> cleared;
		get_tile_rect(*i, x1, y1, x2, y2, cleared);
		if(std::count(cleared.begin(), cleared.end(), "") != cleared.size()) {
			tiles[*i].swap(cleared);
		}
	}
}

void level::clear_tile_rect(int x1, int y1, int x2, int y2)
{
	if(x1 > x2) {
		std::swap(x1, x2);
	}

	if(y1 > y2) {
		std::swap(y1, y2);
	}

	bool changed = false;
	std::vector<std::string> v(1, "");
	for(std::set<int>::const_iterator i = layers_.begin(); i != layers_.end(); ++i) {
		if(hidden_layers_.count(*i)) {
			continue;
		}

		if(add_tile_rect_vector_internal(*i, x1, y1, x2, y2, v)) {
			changed = true;
		}
	}
}

void level::add_tile_solid(const level_tile& t)
{
	//zorders greater than 1000 are considered in the foreground and so
	//have no solids.
	if(t.zorder >= 1000) {
		return;
	}

	if(t.object->width() > widest_tile_) {
		widest_tile_ = t.object->width();
	}

	if(t.object->height() > highest_tile_) {
		highest_tile_ = t.object->height();
	}

	const const_level_object_ptr& obj = t.object;
	if(obj->all_solid()) {
		add_solid_rect(t.x, t.y, t.x + obj->width(), t.y + obj->height(), obj->friction(), obj->traction(), obj->damage(), obj->info());
		return;
	}

	if(obj->has_solid()) {
		for(int y = 0; y != obj->height(); ++y) {
			for(int x = 0; x != obj->width(); ++x) {
				int xpos = x;
				if(!t.face_right) {
					xpos = obj->width() - x - 1;
				}
				if(obj->is_solid(xpos, y)) {
					if(obj->is_passthrough()) {
						add_standable(t.x + x, t.y + y, obj->friction(), obj->traction(), obj->damage(), obj->info());
					} else {
						add_solid(t.x + x, t.y + y, obj->friction(), obj->traction(), obj->damage(), obj->info());
					}
				}
			}
		}
	}
}

struct tile_on_point {
	int x_, y_;
	tile_on_point(int x, int y) : x_(x), y_(y)
	{}

	bool operator()(const level_tile& t) const {
		return x_ >= t.x && y_ >= t.y && x_ < t.x + t.object->width() && y_ < t.y + t.object->height();
	}
};

void level::remove_tiles_at(int x, int y)
{
	tiles_.erase(std::remove_if(tiles_.begin(), tiles_.end(), tile_on_point(x,y)), tiles_.end());
	prepare_tiles_for_drawing();
}

std::vector<point> level::get_solid_contiguous_region(int xpos, int ypos) const
{
	std::vector<point> result;

	xpos = round_tile_size(xpos);
	ypos = round_tile_size(ypos);

	tile_pos base(xpos/TileSize, ypos/TileSize);
	const tile_solid_info* info = solid_.find(base);
	if(info == NULL || info->all_solid == false && info->bitmap.any() == false) {
		return result;
	}

	std::set<tile_pos> positions;
	positions.insert(base);

	int last_count = -1;
	while(positions.size() != last_count) {
		last_count = positions.size();

		std::vector<tile_pos> new_positions;
		foreach(const tile_pos& pos, positions) {
			new_positions.push_back(std::make_pair(pos.first-1, pos.second));
			new_positions.push_back(std::make_pair(pos.first+1, pos.second));
			new_positions.push_back(std::make_pair(pos.first, pos.second-1));
			new_positions.push_back(std::make_pair(pos.first, pos.second+1));
		}

		foreach(const tile_pos& pos, new_positions) {
			if(positions.count(pos)) {
				continue;
			}

			const tile_solid_info* info = solid_.find(pos);
			if(info == NULL || info->all_solid == false && info->bitmap.any() == false) {
				continue;
			}

			positions.insert(pos);
		}
	}

	foreach(const tile_pos& pos, positions) {
		result.push_back(point(pos.first, pos.second));
	}

	return result;
}

const level_tile* level::get_tile_at(int x, int y) const
{
	std::vector<level_tile>::const_iterator i = std::find_if(tiles_.begin(), tiles_.end(), tile_on_point(x,y));
	if(i != tiles_.end()) {
		return &*i;
	} else {
		return NULL;
	}
}

void level::remove_character(entity_ptr e)
{
	if(e->label().empty() == false) {
		chars_by_label_.erase(e->label());
	}
	chars_.erase(std::remove(chars_.begin(), chars_.end(), e), chars_.end());
}

std::vector<entity_ptr> level::get_characters_in_rect(const rect& r, int screen_xpos, int screen_ypos) const
{
	std::vector<entity_ptr> res;
	foreach(entity_ptr c, chars_) {

		const int xP = c->midpoint().x + ((c->parallax_scale_millis_x() - 1000)*screen_xpos)/1000;
		const int yP = c->midpoint().y + ((c->parallax_scale_millis_y() - 1000)*screen_ypos)/1000;
		
		if(point_in_rect(point(xP, yP), r)) {
			res.push_back(c);
		}
	}

	return res;
}

std::vector<entity_ptr> level::get_characters_at_point(int x, int y, int screen_xpos, int screen_ypos) const
{
	std::vector<entity_ptr> result;
	foreach(entity_ptr c, chars_) {
		const int xP = x + ((1000 - (c->parallax_scale_millis_x()))* screen_xpos )/1000;
		const int yP = y + ((1000 - (c->parallax_scale_millis_y()))* screen_ypos )/1000;
		
		if(!c->is_alpha(xP, yP)) {
			result.push_back(c);
		}
	}
	
	return result;
}

entity_ptr level::get_next_character_at_point(int x, int y, int screen_xpos, int screen_ypos) const
{
	std::vector<entity_ptr> v = get_characters_at_point(x, y, screen_xpos, screen_ypos);
	if(v.empty()) {
		return entity_ptr();
	}

	if(editor_selection_.empty()) {
		return v.front();
	}

	std::vector<entity_ptr>::iterator itor = std::find(v.begin(), v.end(), editor_selection_.back());
	if(itor == v.end()) {
		return v.front();
	}

	++itor;
	if(itor == v.end()) {
		itor = v.begin();
	}

	return *itor;
}

void level::add_solid_rect(int x1, int y1, int x2, int y2, int friction, int traction, int damage, const std::string& info_str)
{
	if((x1%TileSize) != 0 || (y1%TileSize) != 0 ||
	   (x2%TileSize) != 0 || (y2%TileSize) != 0) {
		for(int y = y1; y < y2; ++y) {
			for(int x = x1; x < x2; ++x) {
				add_solid(x, y, friction, traction, damage, info_str);
			}
		}

		return;
	}

	for(int y = y1; y < y2; y += TileSize) {
		for(int x = x1; x < x2; x += TileSize) {
			tile_pos pos(x/TileSize, y/TileSize);
			tile_solid_info& s = solid_.insert_or_find(pos);
			s.all_solid = true;
			s.info.friction = friction;
			s.info.traction = traction;

			if(s.info.damage >= 0) {
				s.info.damage = std::min(s.info.damage, damage);
			} else {
				s.info.damage = damage;
			}

			if(info_str.empty() == false) {
				s.info.info = surface_info::get_info_str(info_str);
			}
		}
	}
}

void level::add_solid(int x, int y, int friction, int traction, int damage, const std::string& info)
{
	set_solid(solid_, x, y, friction, traction, damage, info);
}

void level::add_standable(int x, int y, int friction, int traction, int damage, const std::string& info)
{
	set_solid(standable_, x, y, friction, traction, damage, info);
}

void level::set_solid(level_solid_map& map, int x, int y, int friction, int traction, int damage, const std::string& info_str, bool solid)
{
	tile_pos pos(x/TileSize, y/TileSize);
	x = x%TileSize;
	y = y%TileSize;
	if(x < 0) {
		pos.first--;
		x += 32;
	}

	if(y < 0) {
		pos.second--;
		y += 32;
	}
	const int index = y*TileSize + x;
	tile_solid_info& info = map.insert_or_find(pos);

	if(info.info.damage >= 0) {
		info.info.damage = std::min(info.info.damage, damage);
	} else {
		info.info.damage = damage;
	}

	if(solid) {
		info.info.friction = friction;
		info.info.traction = traction;
		info.bitmap.set(index);
	} else {
		if(info.all_solid) {
			info.all_solid = false;
			info.bitmap.set();
		}

		info.bitmap.reset(index);
	}

	if(info_str.empty() == false) {
		info.info.info = surface_info::get_info_str(info_str);
	}
}

void level::add_multi_player(entity_ptr p)
{
	last_touched_player_ = p;
	p->get_player_info()->set_player_slot(players_.size());
	players_.push_back(p);
	chars_.push_back(p);
	if(p->label().empty() == false) {
		chars_by_label_[p->label()] = p;
	}
	layers_.insert(p->zorder());
}

void level::add_player(entity_ptr p)
{
	chars_.erase(std::remove(chars_.begin(), chars_.end(), player_), chars_.end());
	last_touched_player_ = player_ = p;
	if(players_.empty()) {
		player_->get_player_info()->set_player_slot(players_.size());
		players_.push_back(player_);
	} else {
		player_->get_player_info()->set_player_slot(0);
		players_[0] = player_;
	}

	assert(player_);
	chars_.push_back(p);

	//remove objects that have already been destroyed
	const std::vector<int>& destroyed_objects = player_->get_player_info()->get_objects_destroyed(id());
	for(int n = 0; n != chars_.size(); ++n) {
		if(chars_[n]->respawn() == false && std::binary_search(destroyed_objects.begin(), destroyed_objects.end(), chars_[n]->get_id())) {
			if(chars_[n]->label().empty() == false) {
				chars_by_label_.erase(chars_[n]->label());
			}
			chars_[n] = entity_ptr();
		}
	}

	const int difficulty = current_difficulty();
	for(int n = 0; n != chars_.size(); ++n) {
		if(chars_[n].get() != NULL && !chars_[n]->appears_at_difficulty(difficulty)) {
			chars_[n] = entity_ptr();
		}
	}

	chars_.erase(std::remove(chars_.begin(), chars_.end(), entity_ptr()), chars_.end());
}

void level::add_character(entity_ptr p)
{
	if(solid_chars_.empty() == false && p->solid()) {
		solid_chars_.push_back(p);
	}

	if(p->label().empty() == false) {
		chars_by_label_[p->label()] = p;
	}

	if(p->is_human()) {
		add_player(p);
	} else {
		chars_.push_back(p);
	}

	layers_.insert(p->zorder());
}

void level::add_draw_character(entity_ptr p)
{
	active_chars_.push_back(p);
}

void level::force_enter_portal(const portal& p)
{
	entered_portal_active_ = true;
	entered_portal_ = p;
}

const level::portal* level::get_portal() const
{
	if(entered_portal_active_) {
		entered_portal_active_ = false;
		return &entered_portal_;
	}

	if(!player_) {
		return NULL;
	}

	const rect& r = player_->body_rect();
	if(r.x() < boundaries().x() && left_portal_.level_dest.empty() == false) {
		return &left_portal_;
	}

	if(r.x2() > boundaries().x2() && right_portal_.level_dest.empty() == false) {
		return &right_portal_;
	}
	foreach(const portal& p, portals_) {
		if(rects_intersect(r, p.area) && (p.automatic || player_->enter())) {
			return &p;
		}
	}

	return NULL;
}

int level::group_size(int group) const
{
	int res = 0;
	foreach(const entity_ptr& c, active_chars_) {
		if(c->group() == group) {
			++res;
		}
	}

	return res;
}

void level::set_character_group(entity_ptr c, int group_num)
{
	assert(group_num < groups_.size());

	//remove any current grouping
	if(c->group() >= 0) {
		assert(c->group() < groups_.size());
		entity_group& group = groups_[c->group()];
		group.erase(std::remove(group.begin(), group.end(), c), group.end());
	}

	c->set_group(group_num);

	if(group_num >= 0) {
		entity_group& group = groups_[group_num];
		group.push_back(c);
	}
}

int level::add_group()
{
	groups_.resize(groups_.size() + 1);
	return groups_.size() - 1;
}

void level::editor_select_object(entity_ptr c)
{
	if(!c) {
		return;
	}
	editor_selection_.push_back(c);
}

void level::editor_clear_selection()
{
	editor_selection_.clear();
}

const std::string& level::get_background_id() const
{
	if(background_) {
		return background_->id();
	} else {
		static const std::string empty_string;
		return empty_string;
	}
}

void level::set_background_by_id(const std::string& id)
{
	background_ = background::get(id, background_palette_);
}

namespace {

const std::string LevelProperties[] = {
  "cycle", "player", "in_dialog",
  "local_player", "num_active", "active_chars", "chars", "players",
  "in_editor", "zoom", "focus", "gui", "id", "dimensions", "music_volume",
};

enum LEVEL_PROPERTY_ID {
	LEVEL_CYCLE, LEVEL_PLAYER, LEVEL_IN_DIALOG,
	LEVEL_LOCAL_PLAYER, LEVEL_NUM_ACTIVE,
	LEVEL_ACTIVE_CHARS, LEVEL_CHARS, LEVEL_PLAYERS, LEVEL_IN_EDITOR, LEVEL_ZOOM,
	LEVEL_FOCUS, LEVEL_GUI, LEVEL_ID, LEVEL_DIMENSIONS, LEVEL_MUSIC_VOLUME,
};
}

const game_logic::formula_callable_definition& level::get_formula_definition()
{
	static game_logic::formula_callable_definition_ptr result = game_logic::create_formula_callable_definition(LevelProperties, LevelProperties + sizeof(LevelProperties)/sizeof(*LevelProperties));
	return *result;
}

variant level::get_value_by_slot(int slot) const
{
	switch(slot) {
	case LEVEL_CYCLE:
		return variant(cycle_);
	case LEVEL_PLAYER:
		return variant(last_touched_player_.get());
	case LEVEL_IN_DIALOG:
		return variant(in_dialog_);
	case LEVEL_LOCAL_PLAYER:
		return variant(player_.get());
	case LEVEL_NUM_ACTIVE:
		return variant(active_chars_.size());
	case LEVEL_ACTIVE_CHARS: {
		std::vector<variant> v;
		foreach(const entity_ptr& e, active_chars_) {
			v.push_back(variant(e.get()));
		}

		return variant(&v);
	}
	case LEVEL_CHARS: {
		std::vector<variant> v;
		foreach(const entity_ptr& e, active_chars_) {
			v.push_back(variant(e.get()));
		}

		return variant(&v);
	}
	case LEVEL_PLAYERS: {
		std::vector<variant> v;
		foreach(const entity_ptr& e, players()) {
			v.push_back(variant(e.get()));
		}

		return variant(&v);
	}
	case LEVEL_IN_EDITOR:
		return variant(editor_);
	case LEVEL_ZOOM:
		return variant(zoom_level_);
	case LEVEL_FOCUS: {
		std::vector<variant> v;
		foreach(const entity_ptr& e, focus_override_) {
			v.push_back(variant(e.get()));
		}

		return variant(&v);
	}
	case LEVEL_GUI: {
		if(gui_algorithm_) {
			return variant(gui_algorithm_->get_object());
		} else {
			return variant();
		}
	}
	case LEVEL_ID: {
		return variant(id_);
	}
	case LEVEL_DIMENSIONS: {
		std::vector<variant> v;
		v.push_back(variant(boundaries_.x()));
		v.push_back(variant(boundaries_.y()));
		v.push_back(variant(boundaries_.x2()));
		v.push_back(variant(boundaries_.y2()));
		return variant(&v);
	}
	case LEVEL_MUSIC_VOLUME: {
		return variant(sound::get_engine_music_volume());
	}
	}

	ASSERT_LOG(false, "BAD SLOT IN GET_VALUE FROM LEVEL " << slot);
	return variant();
}

variant level::get_value(const std::string& key) const
{
	if(key == "cycle") {
		return variant(cycle_);
	} else if(key == "player") {
		return variant(last_touched_player_.get());
	} else if(key == "in_dialog") {
		return variant(in_dialog_);
	} else if(key == "local_player") {
		return variant(player_.get());
	} else if(key == "num_active") {
		return variant(active_chars_.size());
	} else if(key == "active_chars") {
		std::vector<variant> v;
		foreach(const entity_ptr& e, active_chars_) {
			v.push_back(variant(e.get()));
		}

		return variant(&v);
	} else if(key == "chars") {
		std::vector<variant> v;
		foreach(const entity_ptr& e, chars_) {
			v.push_back(variant(e.get()));
		}

		return variant(&v);
	} else if(key == "in_editor") {
		return variant(editor_);
	} else if(key == "zoom") {
		return variant(zoom_level_);
	} else if(key == "focus") {
		std::vector<variant> v;
		foreach(const entity_ptr& e, focus_override_) {
			v.push_back(variant(e.get()));
		}

		return variant(&v);
	} else if(key == "gui") {
		if(gui_algorithm_) {
			return variant(gui_algorithm_->get_object());
		} else {
			return variant();
		}
	} else if(key == "time_freeze") {
		return variant(time_freeze_);
	} else if(key == "chars_immune_from_freeze") {
		std::vector<variant> v;
		foreach(const entity_ptr& e, chars_immune_from_time_freeze_) {
			v.push_back(variant(e.get()));
		}

		return variant(&v);
	} else if(key == "id") {
		return variant(id());
	} else if(key == "dimensions") {
		std::vector<variant> v;
		v.push_back(variant(boundaries_.x()));
		v.push_back(variant(boundaries_.y()));
		v.push_back(variant(boundaries_.x2()));
		v.push_back(variant(boundaries_.y2()));
		return variant(&v);
	} else if(key == "music_volume") {
		return variant(sound::get_engine_music_volume());
	} else if(key == "segment_width") {
		return variant(segment_width_);
	} else if(key == "segment_height") {
		return variant(segment_height_);
	} else if(key == "num_segments") {
		return variant(sub_levels_.size());
	} else if(key == "camera_position") {
		std::vector<variant> pos;
		pos.reserve(4);
		pos.push_back(variant(last_draw_position().x/100));
		pos.push_back(variant(last_draw_position().y/100));
		pos.push_back(variant(graphics::screen_width()));
		pos.push_back(variant(graphics::screen_height()));
		return variant(&pos);
	} else {
		const_entity_ptr e = get_entity_by_label(key);
		if(e) {
			return variant(e.get());
		}

		return vars_[key];
	}
}

void level::set_value(const std::string& key, const variant& value)
{
	if(key == "lock_screen") {
		if(value.is_list()) {
			lock_screen_.reset(new point(value[0].as_int(), value[1].as_int()));
		} else {
			lock_screen_.reset();
		}
	} else if(key == "zoom") {
		zoom_level_ = value.as_decimal();
	} else if(key == "focus") {
		focus_override_.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			entity* e = value[n].try_convert<entity>();
			if(e) {
				focus_override_.push_back(entity_ptr(e));
			}
		}

		return;
	} else if(key == "in_dialog") {
		in_dialog_ = value.as_bool();
	} else if(key == "time_freeze") {
		time_freeze_ = value.as_int();
	} else if(key == "chars_immune_from_freeze") {
		chars_immune_from_time_freeze_.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			entity_ptr e(value[n].try_convert<entity>());
			if(e) {
				chars_immune_from_time_freeze_.push_back(e);
			}
		}
	} else if(key == "dimensions") {
		ASSERT_EQ(value.num_elements(), 4);
		boundaries_ = rect(value[0].as_int(), value[1].as_int(), value[2].as_int() - value[0].as_int(), value[3].as_int() - value[1].as_int());
	} else if(key == "music_volume") {
		sound::set_engine_music_volume(value.as_decimal().as_float());
	} else if(key == "camera_position") {
		ASSERT_EQ(value.num_elements(), 2);
		last_draw_position().x = value[0].as_int();
		last_draw_position().y = value[1].as_int();
	} else {
		vars_ = vars_.add_attr(variant(key), value);
	}
}

int level::camera_rotation() const
{
	if(!camera_rotation_) {
		return 0;
	}

	return camera_rotation_->execute(*this).as_int();
}

bool level::is_underwater(const rect& r, rect* res_water_area, variant* v) const
{
	return water_ && water_->is_underwater(r, res_water_area, v);
}

void level::get_current(const entity& e, int* velocity_x, int* velocity_y) const
{
	if(e.mass() == 0) {
		return;
	}

	int delta_x = 0, delta_y = 0;
	if(is_underwater(e.body_rect())) {
		delta_x += *velocity_x;
		delta_y += *velocity_y;
		water_->get_current(e, &delta_x, &delta_y);
		delta_x -= *velocity_x;
		delta_y -= *velocity_y;
	}

	delta_x /= e.mass();
	delta_y /= e.mass();

	foreach(const entity_ptr& c, active_chars_) {
		if(c.get() != &e) {
			delta_x += *velocity_x;
			delta_y += *velocity_y;
			c->generate_current(e, &delta_x, &delta_y);
			delta_x -= *velocity_x;
			delta_y -= *velocity_y;
		}
	}

	*velocity_x += delta_x;
	*velocity_y += delta_y;
}

water& level::get_or_create_water()
{
	if(!water_) {
		water_.reset(new water);
	}

	return *water_;
}

entity_ptr level::get_entity_by_label(const std::string& label)
{
	std::map<std::string, entity_ptr>::iterator itor = chars_by_label_.find(label);
	if(itor != chars_by_label_.end()) {
		return itor->second;
	}

	return entity_ptr();
}

const_entity_ptr level::get_entity_by_label(const std::string& label) const
{
	std::map<std::string, entity_ptr>::const_iterator itor = chars_by_label_.find(label);
	if(itor != chars_by_label_.end()) {
		return itor->second;
	}

	return const_entity_ptr();
}

void level::get_all_labels(std::vector<std::string>& labels) const
{
	for(std::map<std::string, entity_ptr>::const_iterator i = chars_by_label_.begin(); i != chars_by_label_.end(); ++i) {
		labels.push_back(i->first);
	}
}

const std::vector<entity_ptr>& level::get_solid_chars() const
{
	if(solid_chars_.empty()) {
		foreach(const entity_ptr& e, chars_) {
			if(e->solid() || e->platform()) {
				solid_chars_.push_back(e);
			}
		}
	}

	return solid_chars_;
}

void level::begin_movement_script(const std::string& key, entity& e)
{
	std::map<std::string, movement_script>::const_iterator itor = movement_scripts_.find(key);
	if(itor == movement_scripts_.end()) {
		return;
	}

	active_movement_scripts_.push_back(itor->second.begin_execution(e));
}

void level::end_movement_script()
{
	if(!active_movement_scripts_.empty()) {
		active_movement_scripts_.pop_back();
	}
}

bool level::can_interact(const rect& body) const
{
	foreach(const portal& p, portals_) {
		if(p.automatic == false && rects_intersect(body, p.area)) {
			return true;
		}
	}

	foreach(const entity_ptr& c, active_chars_) {
		if(c->can_interact_with() && rects_intersect(body, c->body_rect()) &&
		   intersection_rect(body, c->body_rect()).w() >= std::min(body.w(), c->body_rect().w())/2) {
			return true;
		}
	}

	return false;
}

void level::replay_from_cycle(int ncycle)
{
	return;
	const int cycles_ago = cycle_ - ncycle;
	if(cycles_ago <= 0) {
		return;
	}

	int index = static_cast<int>(backups_.size()) - cycles_ago;
	ASSERT_GE(index, 0);

	const int cycle_to_play_until = cycle_;
	restore_from_backup(*backups_[index]);
	ASSERT_EQ(cycle_, ncycle);
	backups_.erase(backups_.begin() + index, backups_.end());
	while(cycle_ < cycle_to_play_until) {
		backup();
		do_processing();
	}
}

void level::backup()
{
	std::map<entity_ptr, entity_ptr> entity_map;

	backup_snapshot_ptr snapshot(new backup_snapshot);
	snapshot->rng_seed = rng::get_seed();
	snapshot->cycle = cycle_;
	snapshot->chars.reserve(chars_.size());


	foreach(const entity_ptr& e, chars_) {
		snapshot->chars.push_back(e->backup());
		entity_map[e] = snapshot->chars.back();

		if(snapshot->chars.back()->is_human()) {
			snapshot->players.push_back(snapshot->chars.back());
			if(e == player_) {
				snapshot->player = snapshot->players.back();
			}
		}
	}

	foreach(entity_group& g, groups_) {
		snapshot->groups.push_back(entity_group());

		foreach(entity_ptr e, g) {
			std::map<entity_ptr, entity_ptr>::iterator i = entity_map.find(e);
			if(i != entity_map.end()) {
				snapshot->groups.back().push_back(i->second);
			}
		}
	}

	foreach(const entity_ptr& e, snapshot->chars) {
		e->map_entities(entity_map);
	}

	snapshot->last_touched_player = last_touched_player_;

	backups_.push_back(snapshot);
	if(backups_.size() > 500) {
		backups_.erase(backups_.begin(), backups_.begin() + 100);
	}
}

void level::reverse_one_cycle()
{
	if(backups_.empty()) {
		return;
	}

	restore_from_backup(*backups_.back());
	backups_.pop_back();
}

void level::restore_from_backup(backup_snapshot& snapshot)
{
	rng::set_seed(snapshot.rng_seed);
	cycle_ = snapshot.cycle;
	chars_ = snapshot.chars;
	players_ = snapshot.players;
	player_ = snapshot.player;
	groups_ = snapshot.groups;
	last_touched_player_ = snapshot.last_touched_player;
	active_chars_.clear();

	solid_chars_.clear();

	chars_by_label_.clear();
	foreach(const entity_ptr& e, chars_) {
		if(e->label().empty() == false) {
			chars_by_label_[e->label()] = e;
		}
	}
}

std::vector<entity_ptr> level::trace_past(entity_ptr e, int ncycles)
{
	int prev_cycle = -1;
	std::vector<entity_ptr> result;
	std::deque<backup_snapshot_ptr>::reverse_iterator i = backups_.rbegin();
	while(i != backups_.rend() && ncycles) {
		const backup_snapshot& snapshot = **i;
		if(prev_cycle != -1 && snapshot.cycle == prev_cycle) {
			++i;
			continue;
		}

		prev_cycle = snapshot.cycle;

		foreach(const entity_ptr& ghost, snapshot.chars) {
			if(ghost->label() == e->label()) {
				result.push_back(ghost);
				break;
			}
		}
		++i;
		--ncycles;
	}

	return result;
}

std::vector<entity_ptr> level::predict_future(entity_ptr e, int ncycles)
{
	const controls::control_backup_scope ctrl_backup_scope;
	backup();
	backup_snapshot_ptr snapshot = backups_.back();
	backups_.pop_back();

	const size_t starting_backups = backups_.size();

	const int controls_end = controls::local_controls_end();
	std::cerr << "PREDICT FUTURE: " << cycle_ << "/" << controls_end << "\n";
	while(cycle_ < controls_end) {
		try {
			const assert_recover_scope safe_scope;
			process();
			backup();
		} catch(validation_failure_exception& e) {
			std::cerr << "ERROR WHILE PREDICTING FUTURE...\n";
			break;
		}
	}

	std::vector<entity_ptr> result = trace_past(e, ncycles);

	backups_.resize(starting_backups);
	restore_from_backup(*snapshot);

	return result;
}

void level::transfer_state_to(level& lvl)
{
	backup();
	lvl.restore_from_backup(*backups_.back());
	backups_.pop_back();
}

void level::get_tile_layers(std::set<int>* all_layers, std::set<int>* hidden_layers)
{
	if(all_layers) {
		foreach(const level_tile& t, tiles_) {
			all_layers->insert(t.zorder);
		}
	}

	if(hidden_layers) {
		*hidden_layers = hidden_layers_;
	}
}

void level::hide_tile_layer(int layer, bool is_hidden)
{
	if(is_hidden) {
		hidden_layers_.insert(layer);
	} else {
		hidden_layers_.erase(layer);
	}
}

void level::editor_freeze_tile_updates(bool value)
{
	if(value) {
		++editor_tile_updates_frozen_;
	} else {
		--editor_tile_updates_frozen_;
		if(editor_tile_updates_frozen_ == 0) {
			rebuild_tiles();
		}
	}
}

decimal level::zoom_level() const
{
	return zoom_level_;
}

void level::add_speech_dialog(boost::shared_ptr<speech_dialog> d)
{
	speech_dialogs_.push(d);
}

void level::remove_speech_dialog()
{
	if(speech_dialogs_.empty() == false) {
		speech_dialogs_.pop();
	}
}

boost::shared_ptr<const speech_dialog> level::current_speech_dialog() const
{
	if(speech_dialogs_.empty()) {
		return boost::shared_ptr<const speech_dialog>();
	}

	return speech_dialogs_.top();
}

bool entity_in_current_level(const entity* e)
{
	const level& lvl = level::current();
	return std::find(lvl.get_chars().begin(), lvl.get_chars().end(), e) != lvl.get_chars().end();
}

void level::add_sub_level(const std::string& lvl, int xoffset, int yoffset, bool add_objects)
{

	const std::map<std::string, sub_level_data>::iterator itor = sub_levels_.find(lvl);
	ASSERT_LOG(itor != sub_levels_.end(), "SUB LEVEL NOT FOUND: " << lvl);

	if(itor->second.active && add_objects) {
		remove_sub_level(lvl);
	}

	const int xdiff = xoffset - itor->second.xoffset;
	const int ydiff = yoffset - itor->second.yoffset;

	itor->second.xoffset = xoffset - itor->second.xbase;
	itor->second.yoffset = yoffset - itor->second.ybase;

	std::cerr << "ADDING SUB LEVEL: " << lvl << "(" << itor->second.lvl->boundaries() << ") " << itor->second.xbase << ", " << itor->second.ybase << " -> " << itor->second.xoffset << ", " << itor->second.yoffset << "\n";

	itor->second.active = true;
	level& sub = *itor->second.lvl;

	if(add_objects) {
		const int difficulty = current_difficulty();
		foreach(entity_ptr e, sub.chars_) {
			if(e->is_human()) {
				continue;
			}
	
			entity_ptr c = e->clone();
			if(!c) {
				continue;
			}

			relocate_object(c, c->x() + itor->second.xoffset, c->y() + itor->second.yoffset);
			if(c->appears_at_difficulty(difficulty)) {
				add_character(c);
				c->handle_event(OBJECT_EVENT_START_LEVEL);

				itor->second.objects.push_back(c);
			}
		}
	}

	foreach(solid_color_rect& r, sub.solid_color_rects_) {
		r.area = rect(r.area.x() + xdiff, r.area.y() + ydiff, r.area.w(), r.area.h());
	}

	build_solid_data_from_sub_levels();
}

void level::remove_sub_level(const std::string& lvl)
{
	const std::map<std::string, sub_level_data>::iterator itor = sub_levels_.find(lvl);
	ASSERT_LOG(itor != sub_levels_.end(), "SUB LEVEL NOT FOUND: " << lvl);

	if(itor->second.active) {
		foreach(entity_ptr& e, itor->second.objects) {
			if(std::find(active_chars_.begin(), active_chars_.end(), e) == active_chars_.end()) {
				remove_character(e);
			}
		}

		itor->second.objects.clear();
	}

	itor->second.active = false;
}

void level::build_solid_data_from_sub_levels()
{
	solid_ = solid_base_;
	standable_ = standable_base_;
	solid_.clear();
	standable_.clear();

	for(std::map<std::string, sub_level_data>::const_iterator i = sub_levels_.begin(); i != sub_levels_.end(); ++i) {
		if(!i->second.active) {
			continue;
		}

		const int xoffset = i->second.xoffset/TileSize;
		const int yoffset = i->second.yoffset/TileSize;
		solid_.merge(i->second.lvl->solid_, xoffset, yoffset);
		standable_.merge(i->second.lvl->standable_, xoffset, yoffset);
	}
}

void level::adjust_level_offset(int xoffset, int yoffset)
{
	game_logic::map_formula_callable* callable(new game_logic::map_formula_callable);
	variant holder(callable);
	callable->add("xshift", variant(xoffset));
	callable->add("yshift", variant(yoffset));
	foreach(entity_ptr e, chars_) {
		e->shift_position(xoffset, yoffset);
		e->handle_event(OBJECT_EVENT_COSMIC_SHIFT, callable);
	}

	boundaries_ = rect(boundaries_.x() + xoffset, boundaries_.y() + yoffset, boundaries_.w(), boundaries_.h());

	for(std::map<std::string, sub_level_data>::iterator i = sub_levels_.begin();
	    i != sub_levels_.end(); ++i) {
		if(i->second.active) {
			add_sub_level(i->first, i->second.xoffset + xoffset + i->second.xbase, i->second.yoffset + yoffset + i->second.ybase, false);
		}
	}

	last_draw_position().x += xoffset*100;
	last_draw_position().y += yoffset*100;
	last_draw_position().focus_x += xoffset;
	last_draw_position().focus_y += yoffset;
}

bool level::relocate_object(entity_ptr e, int new_x, int new_y)
{
	const int orig_x = e->x();
	const int orig_y = e->y();

	const int delta_x = new_x - orig_x;
	const int delta_y = new_y - orig_y;

	e->set_pos(new_x, new_y);

	if(!place_entity_in_level(*this, *e)) {
		//if we can't place the object due to solidity, then cancel
		//the movement.
		e->set_pos(orig_x, orig_y);
		return false;
	}


	//update any x/y co-ordinates to be the same relative to the object's
	//new position.
	if(e->editor_info()) {
		foreach(const editor_variable_info& var, e->editor_info()->vars()) {
			const variant value = e->query_value(var.variable_name());
			switch(var.type()) {
			case editor_variable_info::XPOSITION:
				if(value.is_int()) {
					e->handle_event("editor_changing_variable");
					e->mutate_value(var.variable_name(), variant(value.as_int() + delta_x));
					e->handle_event("editor_changed_variable");
				}
				break;
			case editor_variable_info::YPOSITION:
				if(value.is_int()) {
					e->handle_event("editor_changing_variable");
					e->mutate_value(var.variable_name(), variant(value.as_int() + delta_y));
					e->handle_event("editor_changed_variable");
				}
				break;
			default:
				break;
			}
		}
	}

	return true;
}

void level::record_zorders()
{
	foreach(const level_tile& t, tiles_) {
		t.object->record_zorder(t.zorder);
	}
}

int level::current_difficulty() const
{
	if(!last_touched_player_) {
		return 0;
	}

	playable_custom_object* p = dynamic_cast<playable_custom_object*>(last_touched_player_.get());
	if(!p) {
		return 0;
	}

	return p->difficulty();
}

std::pair<std::vector<level_tile>::const_iterator, std::vector<level_tile>::const_iterator> level::tiles_at_loc(int x, int y) const
{
	x = round_tile_size(x);
	y = round_tile_size(y);

	if(tiles_by_position_.size() != tiles_.size()) {
		tiles_by_position_ = tiles_;
		std::sort(tiles_by_position_.begin(), tiles_by_position_.end(), level_tile_pos_comparer());
	}

	std::pair<int, int> loc(x, y);
	return std::equal_range(tiles_by_position_.begin(), tiles_by_position_.end(), loc, level_tile_pos_comparer());
}


UTILITY(correct_solidity)
{
	std::vector<std::string> files;
	sys::get_files_in_dir(preferences::level_path(), &files);
	foreach(const std::string& file, files) {
		if(file.size() <= 4 || std::string(file.end()-4, file.end()) != ".cfg") {
			continue;
		}

		boost::intrusive_ptr<level> lvl(new level(file));
		lvl->finish_loading();
		lvl->set_as_current_level();

		foreach(entity_ptr c, lvl->get_chars()) {
			if(entity_collides_with_level(*lvl, *c, MOVE_NONE)) {
				if(place_entity_in_level(*lvl, *c)) {
					std::cerr << "LEVEL: " << lvl->id() << " CORRECTED " << c->debug_description() << "\n";
				} else {
					std::cerr << "LEVEL: " << lvl->id() << " FAILED TO CORRECT " << c->debug_description() << "\n";
				}
			}

			c->handle_event("editor_removed");
			c->handle_event("editor_added");
		}

		sys::write_file(preferences::level_path() + file, lvl->write().write_json(true));
	}
}

UTILITY(compile_levels)
{
	preferences::compiling_tiles = true;

	std::cerr << "COMPILING LEVELS...\n";

	std::vector<std::string> files;
	sys::get_files_in_dir(preferences::level_path(), &files);

	variant_builder index_node;

	foreach(const std::string& file, files) {
		std::cerr << "LOADING LEVEL '" << file << "'\n";
		boost::intrusive_ptr<level> lvl(new level(file));
		lvl->finish_loading();
		lvl->record_zorders();
		sys::write_file("data/compiled/level/" + file, lvl->write().write_json(true));

		variant_builder level_summary;
		level_summary.add("level", lvl->id());
		level_summary.add("title", lvl->title());
		level_summary.add("music", lvl->music());
		index_node.add("level", level_summary.build());
	}

	sys::write_file("data/compiled/level_index.cfg", index_node.build().write_json(true));

	level_object::write_compiled();
}

BENCHMARK(level_solid)
{
	//benchmark which tells us how long level::solid takes.
	static level* lvl = new level("stairway-to-heaven.cfg");
	BENCHMARK_LOOP {
		lvl->solid(rng::generate()%1000, rng::generate()%1000);
	}
}

BENCHMARK(load_nene)
{
	BENCHMARK_LOOP {
		level lvl("to-nenes-house.cfg");
	}
}

BENCHMARK(load_all_levels)
{
	std::vector<std::string> files;
	sys::get_files_in_dir(preferences::level_path(), &files);
	BENCHMARK_LOOP {
		foreach(const std::string& file, files) {
			boost::intrusive_ptr<level> lvl(new level(file));
		}
	}
}

BENCHMARK(load_and_save_all_levels)
{
	BENCHMARK_LOOP {
		std::vector<std::string> files;
		sys::get_files_in_dir(preferences::level_path(), &files);
		foreach(const std::string& file, files) {
			std::cerr << "LOAD_LEVEL '" << file << "'\n";
			boost::intrusive_ptr<level> lvl(new level(file));
			lvl->finish_loading();

			sys::write_file(preferences::level_path() + file, lvl->write().write_json(true));
		}
	}
}
