#include <algorithm>
#include <iostream>
#include <math.h>

#include "asserts.hpp"
#include "collision_utils.hpp"
#include "controls.hpp"
#include "draw_scene.hpp"
#include "draw_tile.hpp"
#include "entity.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "gui_formula_functions.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "load_level.hpp"
#include "multiplayer.hpp"
#include "player_info.hpp"
#include "preferences.hpp"
#include "preprocessor.hpp"
#include "random.hpp"
#include "raster.hpp"
#include "stats.hpp"
#include "string_utils.hpp"
#include "tile_map.hpp"
#include "unit_test.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "color_utils.hpp"

namespace {
level* current_level;
}

level& level::current()
{
	ASSERT_LOG(current_level, "Tried to query current level when there is none");
	return *current_level;
}

void level::set_as_current_level()
{
	current_level = this;
}

level::level(const std::string& level_cfg)
	: id_(level_cfg), highlight_layer_(INT_MIN),
	  entered_portal_active_(false), save_point_x_(-1), save_point_y_(-1),
	  editor_(false), show_foreground_(true), show_background_(true), air_resistance_(0), water_resistance_(7), end_game_(false),
      tint_(0), editor_tile_updates_frozen_(0)
{
	std::cerr << "in level constructor...\n";
	const int start_time = SDL_GetTicks();
	turn_reference_counting_off();

	wml::const_node_ptr node(wml::parse_wml(preprocess(sys::read_file("data/level/" + level_cfg))));
	music_ = node->attr("music");
	replay_data_ = node->attr("replay_data");
	cycle_ = wml::get_int(node, "cycle");
	title_ = node->attr("title");
	if(node->has_attr("dimensions")) {
		boundaries_ = rect(node->attr("dimensions"));
	} else {
		boundaries_ = rect(0, 0, wml::get_int(node, "width", 800), wml::get_int(node, "height", 600));
	}

	if(node->has_attr("lock_screen")) {
		lock_screen_.reset(new point(node->attr("lock_screen")));
	}

	xscale_ = wml::get_int(node, "xscale", 100);
	yscale_ = wml::get_int(node, "yscale", 100);
	auto_move_camera_ = point(node->attr("auto_move_camera"));
	air_resistance_ = wml::get_int(node, "air_resistance", 20);
	water_resistance_ = wml::get_int(node, "water_resistance", 100);

	camera_rotation_ = game_logic::formula::create_optional_formula(node->attr("camera_rotation"));

	preloads_ = util::split(node->attr("preloads"));

	wml::node::const_child_iterator r1 = node->begin_child("solid_rect");
	wml::node::const_child_iterator r2 = node->end_child("solid_rect");
	for(; r1 != r2; ++r1) {
		solid_rect r;
		r.r = rect(r1->second->attr("rect"));
		r.friction = wml::get_int(r1->second, "friction", 100);
		r.traction = wml::get_int(r1->second, "traction", 100);
		r.damage = wml::get_int(r1->second, "damage");
		solid_rects_.push_back(r);
		add_solid_rect(r.r.x(), r.r.y(), r.r.x2(), r.r.y2(), r.friction, r.traction, r.damage);
	}

	std::cerr << "building..." << SDL_GetTicks() << "\n";
	wml::node::const_child_iterator t1 = node->begin_child("tile");
	wml::node::const_child_iterator t2 = node->end_child("tile");
	widest_tile_ = 0;
	highest_tile_ = 0;
	layers_.insert(0);
	for(; t1 != t2; ++t1) {
		const level_tile t = level_object::build_tile(t1->second);
		tiles_.push_back(t);
		layers_.insert(t.zorder);
		add_tile_solid(t);
	}
	std::cerr << "done building..." << SDL_GetTicks() << "\n";

	t1 = node->begin_child("tile_map");
	t2 = node->end_child("tile_map");
	int begin_tile_index = tiles_.size();
	for(; t1 != t2; ++t1) {
		tile_map m(t1->second);
		tile_maps_[m.zorder()] = m;
		const int before = tiles_.size();
		tile_maps_[m.zorder()].build_tiles(&tiles_);
		std::cerr << "LAYER " << m.zorder() << " BUILT " << (tiles_.size() - before) << " tiles\n";
	}

	std::cerr << "done building tile_map..." << SDL_GetTicks() << "\n";

	for(int i = begin_tile_index; i != tiles_.size(); ++i) {
		add_tile_solid(tiles_[i]);
		layers_.insert(tiles_[i].zorder);
	}

	std::sort(tiles_.begin(), tiles_.end(), level_tile_zorder_pos_comparer());
	prepare_tiles_for_drawing();

	wml::node::const_child_iterator c1 = node->begin_child("character");
	wml::node::const_child_iterator c2 = node->end_child("character");
	for(; c1 != c2; ++c1) {
		if(c1->second->get_child("type")) {
			wml_chars_.push_back(c1->second);
			continue;
		}

		load_character(c1->second);
	}

	wml::node::const_child_iterator p1 = node->begin_child("portal");
	wml::node::const_child_iterator p2 = node->end_child("portal");
	for(; p1 != p2; ++p1) {
		portal p;
		p.area = rect(p1->second->attr("rect"));
		p.level_dest = p1->second->attr("level");
		p.dest = point(p1->second->attr("dest"));
		p.dest_starting_pos = p1->second->attr("dest_starting_pos").str() == "yes";
		p.automatic = wml::get_bool(p1->second, "automatic", true);
		p.transition = p1->second->attr("transition");
		portals_.push_back(p);
	}

	if(node->has_attr("next_level")) {
		right_portal_.level_dest = node->attr("next_level");
		right_portal_.dest_str = "left";
		right_portal_.dest_starting_pos = false;
		right_portal_.automatic = true;
	}

	if(node->has_attr("previous_level")) {
		left_portal_.level_dest = node->attr("previous_level");
		left_portal_.dest_str = "right";
		left_portal_.dest_starting_pos = false;
		left_portal_.automatic = true;
	}

	wml::const_node_ptr bg = node->get_child("background");
	if(bg) {
		background_.reset(new background(bg));
	} else if(node->has_attr("background")) {
		background_ = background::get(node->attr("background"));
	}

	wml::const_node_ptr water_node = node->get_child("water");
	if(water_node) {
		water_.reset(new water(water_node));
	}

	FOREACH_WML_CHILD(script_node, node, "script") {
		movement_script s(script_node);
		movement_scripts_[s.id()] = s;
	}

	const int time_taken_ms = (SDL_GetTicks() - start_time);
	stats::record_event(id(), stats::const_record_ptr(new stats::load_level_record(time_taken_ms)));
	std::cerr << "done level constructor: " << time_taken_ms << "\n";

	gui_algorithm_ = gui_algorithm::create(wml::get_str(node, "gui", "default"));
}

void level::load_character(wml::const_node_ptr c)
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
	foreach(wml::const_node_ptr node, wml_chars_) {
		load_character(node);
	}

	wml_chars_.clear();

	controls::new_level(cycle_, players_.empty() ? 1 : players_.size(), multiplayer::slot());
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

void level::rebuild_tiles()
{
	if(editor_tile_updates_frozen_) {
		return;
	}

	std::cerr << "rebuild tiles...\n";
	solid_.clear();
	standable_.clear();
	tiles_.clear();
	for(std::map<int, tile_map>::const_iterator i = tile_maps_.begin(); i != tile_maps_.end(); ++i) {
		std::cerr << "build tiles...\n";
		i->second.build_tiles(&tiles_);
	}

	std::cerr << "adding solids...\n";

	foreach(level_tile& t, tiles_) {
		add_tile_solid(t);
		layers_.insert(t.zorder);
	}

	std::cerr << "sorting...\n";

	std::sort(tiles_.begin(), tiles_.end(), level_tile_zorder_pos_comparer());
	prepare_tiles_for_drawing();
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

	std::sort(tiles_.begin(), tiles_.end(), level_tile_zorder_pos_comparer());
	prepare_tiles_for_drawing();
}

wml::node_ptr level::write() const
{
	wml::node_ptr res(new wml::node("level"));
	res->set_attr("title", title_);
	res->set_attr("music", music_);
	if(cycle_) {
		res->set_attr("cycle", formatter() << cycle_);
	}
	res->set_attr("dimensions", boundaries().to_string());

	res->set_attr("xscale", formatter() << xscale_);
	res->set_attr("yscale", formatter() << yscale_);
	res->set_attr("auto_move_camera", auto_move_camera_.to_string());
	res->set_attr("air_resistance", formatter() << air_resistance_);
	res->set_attr("water_resistance", formatter() << water_resistance_);

	res->set_attr("preloads", util::join(preloads_));

	if(lock_screen_) {
		res->set_attr("lock_screen", lock_screen_->to_string());
	}

	if(water_) {
		res->add_child(water_->write());
	}

	if(camera_rotation_) {
		res->set_attr("camera_rotation", camera_rotation_->str());
	}

	foreach(const solid_rect& r, solid_rects_) {
		wml::node_ptr node(new wml::node("solid_rect"));
		node->set_attr("rect", r.r.to_string());
		node->set_attr("friction", formatter() << r.friction);
		node->set_attr("traction", formatter() << r.traction);
		node->set_attr("damage", formatter() << r.damage);
		res->add_child(node);
	}

	for(std::map<int, tile_map>::const_iterator i = tile_maps_.begin(); i != tile_maps_.end(); ++i) {
		res->add_child(i->second.write());
	}
/*
	foreach(const level_tile& tile, tiles_) {
		wml::node_ptr t(new wml::node("tile"));
		t->set_attr("x", formatter() << tile.x);
		t->set_attr("y", formatter() << tile.y);
		t->set_attr("zorder", formatter() << tile.zorder);
		t->set_attr("tile", formatter() << tile.object->id());
		t->set_attr("face_right", tile.face_right ? "true" : "false");
		res->add_child(t);
	}
	*/

	foreach(entity_ptr ch, chars_) {
		res->add_child(ch->write());
	}

	foreach(const portal& p, portals_) {
		wml::node_ptr node(new wml::node("portal"));
		node->set_attr("rect", p.area.to_string());
		node->set_attr("level", p.level_dest);
		node->set_attr("dest_starting_pos", p.dest_starting_pos ? "yes" : "no");
		node->set_attr("dest", p.dest.to_string());
		node->set_attr("automatic", p.automatic ? "yes" : "no");
		node->set_attr("transition", p.transition);
		res->add_child(node);
	}

	if(right_portal_.level_dest.empty() == false) {
		res->set_attr("next_level", right_portal_.level_dest);
	}

	std::cerr << "PREVIOUS LEVEL: " << left_portal_.level_dest << "\n";
	if(left_portal_.level_dest.empty() == false) {
		res->set_attr("previous_level", left_portal_.level_dest);
	}

	if(background_) {
		if(background_->id().empty()) {
			res->add_child(background_->write());
		} else {
			res->set_attr("background", background_->id());
		}
	}

	for(std::map<std::string, movement_script>::const_iterator i = movement_scripts_.begin(); i != movement_scripts_.end(); ++i) {
		res->add_child(i->second.write());
	}

	return res;
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
	itor t = std::lower_bound(range.first, range.second, y,
	                          level_tile_y_pos_comparer());

	short begin_range = 0;
	short end_range = 0;

	layer_blit_info& blit_info = blit_cache_[layer];

	const rect tile_positions(x/32 - (x < 0 ? 1 : 0), y/32 - (y < 0 ? 1 : 0),
	                          (x + w)/32 - (x + w < 0 ? 1 : 0),
							  (y + h)/32 - (y + h < 0 ? 1 : 0));

	graphics::blit_queue& blit_queue_store = blit_info.blit_queue;

	if(blit_info.tile_positions != tile_positions) {
		blit_info.tile_positions = tile_positions;
		blit_queue_store.clear();

		const graphics::blit_queue* blit_queue = NULL;

		while(t != tiles_.end() && t->zorder == layer && t->y <= y + h) {
			const int increment = 8;
			while(t->x < x - TileSize) {
				if(t+increment >= tiles_.end() || t[increment].y != t->y || t[increment].zorder != t->zorder || t[increment].x > x - TileSize) {
					break;
				}

				t += increment;
			}

			while(t->x > x + w) {
				if(t+increment >= tiles_.end() || t[increment].y != t->y || t[increment].zorder != t->zorder) {
					break;
				}

				t += increment;
			}

			if(t->x >= x && t->x <= x + w && !t->draw_disabled) {
				if(blit_queue != t->blit_queue) {
					if(blit_queue) {
						if(!blit_queue_store.merge(*blit_queue, begin_range, end_range)) {
							blit_info.tile_positions = rect();
							blit_queue_store.do_blit();
							blit_queue_store.clear();
							blit_queue_store.merge(*blit_queue, begin_range, end_range);
						}
//						blit_queue->do_blit_range(begin_range, end_range);
					}

					blit_queue = t->blit_queue;
					begin_range = t->blit_queue_begin;
				}

				end_range = t->blit_queue_end;
			}
			++t;
		}

		if(blit_queue) {
						if(!blit_queue_store.merge(*blit_queue, begin_range, end_range)) {
							blit_info.tile_positions = rect();
							blit_queue_store.do_blit();
							blit_queue_store.clear();
							blit_queue_store.merge(*blit_queue, begin_range, end_range);
						}
//			blit_queue->do_blit_range(begin_range, end_range);
		}
	}

	blit_queue_store.do_blit();

	draw_layer_solid(layer, x, y, w, h);

	glPopMatrix();

	glColor4f(1.0, 1.0, 1.0, 1.0);
}

void level::draw_layer_solid(int layer, int x, int y, int w, int h) const
{
	typedef std::vector<solid_color_rect>::const_iterator SolidItor;
	std::pair<SolidItor, SolidItor> solid = std::equal_range(solid_color_rects_.begin(), solid_color_rects_.end(), layer, solid_color_rect_cmp());
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
	}
}

void level::prepare_tiles_for_drawing()
{
	solid_color_rects_.clear();

	for(int n = 0; n != tiles_.size(); ++n) {
		if(tiles_[n].object->solid_color()) {
			tiles_[n].draw_disabled = true;
			tiles_[n].blit_queue = NULL;
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

		tiles_[n].draw_disabled = false;
		tiles_[n].blit_queue_buf.clear();

		//see if this tile can be drawn in a strip with the previous tile.
		if(n > 0 && tiles_[n].x == tiles_[n-1].x + TileSize && tiles_[n].y == tiles_[n-1].y && tiles_[n].object->texture().get_id() == tiles_[n-1].object->texture().get_id() && !tiles_[n-1].draw_disabled) {
			tiles_[n].blit_queue = tiles_[n-1].blit_queue;
		} else {
			tiles_[n].blit_queue = &tiles_[n].blit_queue_buf;
		}

		tiles_[n].blit_queue = &tiles_[n].blit_queue_buf;
		tiles_[n].blit_queue_begin = tiles_[n].blit_queue->position();
		queue_draw_tile(*tiles_[n].blit_queue, tiles_[n]);
		tiles_[n].blit_queue_end = tiles_[n].blit_queue->position();
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
	return a->zorder() < b->zorder() ||
	       a->zorder() == b->zorder() && a->y() < b->y() ||
		   a->zorder() == b->zorder() && a->y() < b->y() ||
		   a->zorder() == b->zorder() && a->y() == b->y() && a->x() < b->x();
}
}

void level::draw_status() const
{
	if(gui_algorithm_) {
		gui_algorithm_->draw(*this);
	}
}

void level::draw(int x, int y, int w, int h) const
{
	++draw_count;

	const int start_x = x;
	const int start_y = y;

	const int ticks = SDL_GetTicks();
	x -= widest_tile_;
	y -= highest_tile_;
	w += widest_tile_;
	h += highest_tile_;

	const std::vector<entity_ptr>* chars_ptr = &active_chars_;
	std::vector<entity_ptr> editor_chars_buf;
	if(editor_) {
		//in the editor we draw all chars, not just active chars. We also
		//sort the chars by drawing order to make sure they are drawn in
		//the correct order.
		editor_chars_buf = chars_;
		std::sort(editor_chars_buf.begin(), editor_chars_buf.end(), sort_entity_drawing_pos);
		chars_ptr = &editor_chars_buf;
	}

	const std::vector<entity_ptr>& chars = *chars_ptr;

	std::vector<entity_ptr>::const_iterator entity_itor = chars.begin();

	bool water_drawn = true;
	int water_zorder = 0;
	if(water_) {
		water_drawn = false;
		water_zorder = water_->zorder();
	}

	std::set<int>::const_iterator layer = layers_.begin();

	for(; layer != layers_.end() && *layer < 0; ++layer) {

		if(!water_drawn && *layer > water_zorder) {
			water_->draw(x, y, w, h);
			water_drawn = true;
		}

		while(entity_itor != chars.end() && (*entity_itor)->zorder() <= *layer) {
			if(!(*entity_itor)->is_human()) {
				(*entity_itor)->draw();
				if(editor_) {
					(*entity_itor)->draw_group();
				}
			}
			++entity_itor;
		}

		draw_layer(*layer, x, y, w, h);
	}

	foreach(const const_entity_ptr& p, players_) {
		p->draw();
	}

	for(; layer != layers_.end(); ++layer) {
		if(!water_drawn && *layer > water_zorder) {
			water_->draw(x, y, w, h);
			water_drawn = true;
		}

		while(entity_itor != chars.end() && (*entity_itor)->zorder() <= *layer) {
			if(!(*entity_itor)->is_human()) {
				(*entity_itor)->draw();
				if(editor_) {
					(*entity_itor)->draw_group();
				}
			}
			++entity_itor;
		}

		draw_layer(*layer, x, y, w, h);
	}

	if(!water_drawn && *layer > water_zorder) {
		water_->draw(x, y, w, h);
			water_drawn = true;
	}

	while(entity_itor != chars.end()) {
		if(!(*entity_itor)->is_human()) {
			(*entity_itor)->draw();
			if(editor_) {

				(*entity_itor)->draw_group();
			}
		}
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
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		const GLfloat alpha = 0.5 + sin(draw_count/5.0)*0.5;
		glColor4f(1.0, 1.0, 1.0, alpha);

		if(editor_highlight_) {
			editor_highlight_->draw();
		}

		foreach(const entity_ptr& e, editor_selection_) {
			e->draw();
		}

		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	draw_debug_solid(x, y, w, h);

	if(background_) {
		background_->draw_foreground(start_x, start_y, 0.0, cycle());
	}

	if( tint_.a() > 0){
		graphics::draw_rect(rect(x,y,w,h), tint_ );
	}
}

void level::draw_debug_solid(int x, int y, int w, int h) const
{
	if(preferences::show_debug_hitboxes() == false) {
		return;
	}
}

void level::draw_background(double x, double y, int rotation) const
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
		background_->draw(x, y, rotation, cycle());
	}
}

void level::process()
{
	if(gui_algorithm_) {
		gui_algorithm_->process(*this);
	}

	multiplayer::send_and_receive();

	const int LevelPreloadFrequency = 500; //10 seconds
	//see if we have levels to pre-load. Load one periodically.
	if((cycle_%LevelPreloadFrequency) == 0) {
		const int index = cycle_/LevelPreloadFrequency;
		if(index < preloads_.size()) {
			preload_level(preloads_[index]);
		}
	}

	controls::read_local_controls();

	do_processing();
}

void level::do_processing()
{
	++cycle_;

	const int ticks = SDL_GetTicks();
	active_chars_.clear();

	if(!player_) {
		return;
	}

	detect_user_collisions(*this);

	const int screen_left = last_draw_position().x/100;
	const int screen_right = last_draw_position().x/100 + graphics::screen_width();
	const int screen_top = last_draw_position().y/100;
	const int screen_bottom = last_draw_position().y/100 + graphics::screen_height();

	foreach(entity_ptr& c, chars_) {
		int distance_x = 0, distance_y = 0;
		c->activation_distance(&distance_x, &distance_y);

		const int x = c->x();
		const int y = c->y();
		const int x2 = x + c->current_frame().width();
		const int y2 = y + c->current_frame().height();

		bool is_active = c->always_active();
		if(!is_active && players_.size() == 1) {
			//in single player mode, make objects on-screen always active.
			//we don't have a good solution for this for multiplayer, yet.
			if(x2 >= screen_left && x < screen_right && y2 >= screen_top && y < screen_bottom) {
				is_active = true;
			}
		}

		if(!is_active) {
			foreach(const entity_ptr& p, players_) {
				if((p->x() < x ? x - p->x() : p->x() - x2) < distance_x &&
			   	   (p->y() < y ? y - p->y() : p->y() - y2) < distance_y) {
					is_active = true;
					break;
				}
			}
		}

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

	//std::cerr << "active: " << active_chars_.size() << "/" << chars_.size() << "\n";

	const int ActivationDistance = 700;

	foreach(entity_ptr c, active_chars_) {
		c->process(*this);
		if(c->destroyed() && !c->is_human()) {
			std::cerr << "OBJECT DIE: " << c->get_id() << "\n";
			if(player_ && c->get_id() != -1) {
			std::cerr << "OBJECT DEST: " << c->get_id() << "\n";
				player_->is_human()->object_destroyed(id(), c->get_id());
			}

			erase_char(c);
		}
	}

	//std::cerr << "process: " << (SDL_GetTicks() - ticks) << "\n";

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

bool level::is_solid(const level_solid_map& map, const entity& e, const std::vector<point>& points, int* friction, int* traction, int* damage) const
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
				if(friction) {
					*friction = info->friction;
				}

				if(traction) {
					*traction = info->traction;
				}

				if(damage) {
					*damage = info->damage;
				}
				return true;
			}
		
			const int index = y*TileSize + x;
			if(info->bitmap.test(index)) {
				if(friction) {
					*friction = info->friction;
				}

				if(traction) {
					*traction = info->traction;
				}

				if(damage) {
					*damage = info->damage;
				}
				return true;
			}
		}

		prev_x = x;
		prev_y = y;
	}

	return false;
}

bool level::is_solid(const level_solid_map& map, int x, int y, int* friction, int* traction, int* damage) const
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
			if(friction) {
				*friction = info->friction;
			}

			if(traction) {
				*traction = info->traction;
			}

			if(damage) {
				*damage = info->damage;
			}
			return true;
		}
		
		const int index = y*TileSize + x;
		if(info->bitmap.test(index)) {
			if(friction) {
				*friction = info->friction;
			}

			if(traction) {
				*traction = info->traction;
			}

			if(damage) {
				*damage = info->damage;
			}
			return true;
		} else {
			return false;
		}
	}

	return false;
}

bool level::standable(int x, int y, int* friction, int* traction, int* damage) const
{
	if(is_solid(solid_, x, y, friction, traction, damage) ||
	   is_solid(standable_, x, y, friction, traction, damage)) {
	   return true;
	}

	return false;
}

bool level::standable_tile(int x, int y, int* friction, int* traction, int* damage) const
{
	if(is_solid(solid_, x, y, friction, traction, damage) ||
	   is_solid(standable_, x, y, friction, traction, damage)) {
		return true;
	}
	
	return false;
}


bool level::solid(int x, int y, int* friction, int* traction, int* damage) const
{
	return is_solid(solid_, x, y, friction, traction, damage);
}

bool level::solid(const entity& e, const std::vector<point>& points, int* friction, int* traction, int* damage) const
{
	return is_solid(solid_, e, points, friction, traction, damage);
}

bool level::solid(const rect& r, int* friction, int* traction, int* damage) const
{
	//TODO: consider optimizing this function.
	const int ybegin = r.y();
	const int yend = r.y2();
	const int xbegin = r.x();
	const int xend = r.x2();

	for(int y = ybegin; y != yend; ++y) {
		for(int x = xbegin; x != xend; ++x) {
			if(solid(x, y, friction, traction, damage)) {
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
	for(int y = r.y(); y < r.y2(); ++y) {
		for(int x = r.x(); x < r.x2(); ++x) {
			set_solid(solid_, x, y, 0, 0, 0, solid);
		}
	}
}

entity_ptr level::collide(int x, int y, const entity* exclude) const
{
	if(editor_) {
		return entity_ptr();
	}
	
	const bool is_players_side = exclude && (exclude->on_players_side() || exclude->is_human());
	entity_ptr res;
	for(std::vector<entity_ptr>::const_iterator i = chars_.begin();
	    i != chars_.end(); ++i) {
		const entity_ptr& c = *i;
		if(c->destroyed()) {
			continue;
		}
		if(is_players_side && c->on_players_side()) {
			continue;
		}
		if(c.get() != exclude && !c->is_human() &&
		   (!c->body_passthrough() || is_players_side && c->body_harmful()) &&
		   c->point_collides(x,y)) {
			res = c;
			if(res->body_harmful()) {
				return res;
			}
		}

		if(c.get() != exclude) {
			if((!c->body_passthrough() || is_players_side && c->body_harmful()) &&
			   c->point_collides(x,y)) {
				return c;
			}

			if(point_in_rect(point(x,y), c->hit_rect())) {
				return c;
			}
		}
	}

	return res;
}

entity_ptr level::collide(const rect& r, const entity* exclude) const
{
	if(editor_) {
		return entity_ptr();
	}

	const bool is_players_side = exclude && (exclude->on_players_side() || exclude->is_human());
	for(std::vector<entity_ptr>::const_iterator i = chars_.begin();
	    i != chars_.end(); ++i) {
		const entity_ptr& c = *i;
		if(c->destroyed()) {
			continue;
		}
		if(is_players_side && c->on_players_side()) {
			continue;
		}

		if(c.get() != exclude && !c->is_human()) {
			if((!c->body_passthrough() || is_players_side && c->body_harmful()) &&
			   c->rect_collides(r)) {
				return c;
			}

			if(rects_intersect(r, c->hit_rect())) {
				return c;
			}
		}
	}

	return entity_ptr();
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

entity_ptr level::hit_by_player(const rect& r) const
{
	for(std::vector<entity_ptr>::const_iterator p = players_.begin();
	    p != players_.end(); ++p) {
		if(rects_intersect(r, (*p)->hit_rect())) {
			return *p;
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
	rebuild_tiles_rect(rect(x1-64, y1-128, (x2 - x1) + 128, (y2 - y1) + 256));
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
		std::cerr << "x: " << x << "\n";
		for(int y = y1; y < y2; y += 32) {
			std::cerr << "adding tile: " << x << "," << y << ": (" << tiles[index] << ")\n";
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
		add_solid_rect(t.x, t.y, t.x + obj->width(), t.y + obj->height(), obj->friction(), obj->traction(), obj->damage());
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
						add_standable(t.x + x, t.y + y, obj->friction(), obj->traction(), obj->damage());
					} else {
						add_solid(t.x + x, t.y + y, obj->friction(), obj->traction(), obj->damage());
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
	std::cerr << "removing char: '" << e->label() << "'\n";
	if(e->label().empty() == false) {
		chars_by_label_.erase(e->label());
	}
	chars_.erase(std::remove(chars_.begin(), chars_.end(), e), chars_.end());
	std::cerr << "removed char: '" << e->label() << "'\n";
}

std::vector<entity_ptr> level::get_characters_in_rect(const rect& r) const
{
	std::vector<entity_ptr> res;
	foreach(entity_ptr c, chars_) {
		if(point_in_rect(point(c->x(), c->y()), r)) {
			res.push_back(c);
		}
	}

	return res;
}

entity_ptr level::get_character_at_point(int x, int y) const
{
	foreach(entity_ptr c, chars_) {
		const int xpos = x - c->x();
		const int ypos = y - c->y();
		if(!c->is_alpha(x, y)) {
			return c;
		}
	}
	
	return entity_ptr();
}

void level::add_solid_rect(int x1, int y1, int x2, int y2, int friction, int traction, int damage)
{
	if((x1%TileSize) != 0 || (y1%TileSize) != 0 ||
	   (x2%TileSize) != 0 || (y2%TileSize) != 0) {
		for(int y = y1; y < y2; ++y) {
			for(int x = x1; x < x2; ++x) {
				add_solid(x, y, friction, traction, damage);
			}
		}

		return;
	}

	for(int y = y1; y < y2; y += TileSize) {
		for(int x = x1; x < x2; x += TileSize) {
			tile_pos pos(x/TileSize, y/TileSize);
			tile_solid_info& s = solid_.insert_or_find(pos);
			s.all_solid = true;
			s.friction = friction;
			s.traction = traction;
			s.damage = damage;
		}
	}
}

void level::add_solid(int x, int y, int friction, int traction, int damage)
{
	set_solid(solid_, x, y, friction, traction, damage);
}

void level::add_standable(int x, int y, int friction, int traction, int damage)
{
	set_solid(standable_, x, y, friction, traction, damage);
}

void level::set_solid(level_solid_map& map, int x, int y, int friction, int traction, int damage, bool solid)
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

	if(solid) {
		info.friction = friction;
		info.traction = traction;
		info.damage = damage;
		info.bitmap.set(index);
	} else {
		if(info.all_solid) {
			info.all_solid = false;
			info.bitmap.set();
		}

		info.bitmap.reset(index);
	}
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
			std::cerr << "removing character: " << n << ": " << chars_[n]->get_id() << "\n";
			if(chars_[n]->label().empty() == false) {
				chars_by_label_.erase(chars_[n]->label());
			}
			chars_[n] = entity_ptr();
		}
	}

	chars_.erase(std::remove(chars_.begin(), chars_.end(), entity_ptr()), chars_.end());
}

void level::add_character(entity_ptr p)
{
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
	background_ = background::get(id);
}

variant level::get_value(const std::string& key) const
{
	if(key == "cycle") {
		return variant(cycle_);
	} else if(key == "player") {
		return variant(last_touched_player_.get());
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
	} else if(key == "tint") {
		return variant(new graphics::color(tint_));
	} else if(key == "in_editor") {
		return variant(editor_);
	} else {
		const_entity_ptr e = get_entity_by_label(key);
		if(e) {
			return variant(e.get());
		}

		std::map<std::string, variant>::const_iterator i = vars_.find(key);
		if(i != vars_.end()) {
			return i->second;
		}

		return variant();
	}
}

void level::set_value(const std::string& key, const variant& value)
{
	if(key == "tint") {
		 if(value.is_string()) {
			tint_ = graphics::color(value.as_string());
		 } else if(value.is_list() && value.num_elements() == 4) {
			tint_ = graphics::color(value[0].as_int(), value[1].as_int(), value[2].as_int(), value[3].as_int());
		 }
	} else if(key == "lock_screen") {
		if(value.is_list()) {
			lock_screen_.reset(new point(value[0].as_int(), value[1].as_int()));
		} else {
			lock_screen_.reset();
		}
	} else {
		vars_[key] = value;
	}
}

int level::camera_rotation() const
{
	if(!camera_rotation_) {
		return 0;
	}

	return camera_rotation_->execute(*this).as_int();
}

bool level::is_underwater(const rect& r, rect* res_water_area) const
{
	return water_ && water_->is_underwater(r, res_water_area);
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

	std::cerr << "CURRENT: " << delta_x << "," << delta_y << "\n";

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
	std::cerr << "REPLAY FROM " << ncycle << "-" << (cycle_-1) << "\n";

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

	std::cerr << "BACKUP " << cycle_ << ": ";
	backup_snapshot_ptr snapshot(new backup_snapshot);
	snapshot->rng_seed = rng::get_seed();
	snapshot->cycle = cycle_;
	snapshot->chars.reserve(chars_.size());
	foreach(const entity_ptr& e, chars_) {
		std::cerr << e->debug_description() << "(" << (e->is_human() ? "HUMAN," : "") << e->centi_x() << "," << e->centi_y() << "):";
		snapshot->chars.push_back(e->backup());
		entity_map[e] = snapshot->chars.back();

		if(snapshot->chars.back()->is_human()) {
			snapshot->players.push_back(snapshot->chars.back());
			if(e == player_) {
				snapshot->player = snapshot->players.back();
			}
		}
	}

	foreach(const entity_ptr& e, snapshot->chars) {
		e->map_entities(entity_map);
	}

	std::cerr << "\n";

	snapshot->last_touched_player = last_touched_player_;

	backups_.push_back(snapshot);
	if(backups_.size() > 300) {
		backups_.erase(backups_.begin(), backups_.begin() + 100);
	}
}

void level::restore_from_backup(backup_snapshot& snapshot)
{
	rng::set_seed(snapshot.rng_seed);
	cycle_ = snapshot.cycle;
	chars_ = snapshot.chars;
	players_ = snapshot.players;
	player_ = snapshot.player;
	last_touched_player_ = snapshot.last_touched_player;

	solid_chars_.clear();

	chars_by_label_.clear();
	foreach(const entity_ptr& e, chars_) {
		if(e->label().empty() == false) {
			chars_by_label_[e->label()] = e;
		}
	}
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

BENCHMARK(level_solid)
{
	//benchmark which tells us how long level::solid takes.
	static level* lvl = new level("stairway-to-heaven.cfg");
	BENCHMARK_LOOP {
		lvl->solid(rng::generate()%1000, rng::generate()%1000);
	}
}

BENCHMARK(load_fatpipe)
{
	BENCHMARK_LOOP {
		level lvl("fatpipe-stones.cfg");
	}
}

#include "wml_writer.hpp"

BENCHMARK(load_and_save_all_levels)
{
	BENCHMARK_LOOP {
		std::vector<std::string> files;
		sys::get_files_in_dir("data/level/", &files);
		foreach(const std::string& file, files) {
			std::cerr << "LOAD_LEVEL '" << file << "'\n";
			boost::intrusive_ptr<level> lvl(new level(file));

			std::string data;
			wml::write(lvl->write(), data);
			sys::write_file("data/level/" + file, data);
		}
	}
}
