#include "graphics.hpp"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <stdio.h>

#include <cassert>
#include <iostream>

#include "asserts.hpp"
#include "collision_utils.hpp"
#include "custom_object.hpp"
#include "custom_object_callable.hpp"
#include "custom_object_functions.hpp"
#include "draw_scene.hpp"
#include "font.hpp"
#include "formatter.hpp"
#include "formula_callable.hpp"
#include "formula_profiler.hpp"
#include "graphical_font.hpp"
#include "level.hpp"
#include "level_logic.hpp"
#include "object_events.hpp"
#include "playable_custom_object.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "string_utils.hpp"
#include "surface_formula.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "unit_test.hpp"
#include "utils.hpp"
#include "sound.hpp"

namespace {
const game_logic::formula_variable_storage_ptr& global_vars()
{
	static game_logic::formula_variable_storage_ptr obj(new game_logic::formula_variable_storage);
	return obj;
}
}

struct custom_object_text {
	std::string text;
	const_graphical_font_ptr font;
	int size;
	bool centered;
	rect dimensions;
	int alpha;
};

custom_object::custom_object(wml::const_node_ptr node)
  : entity(node),
    previous_y_(y()),
	custom_type_(node->get_child("type")),
    type_(custom_type_ ?
	      const_custom_object_type_ptr(new custom_object_type(custom_type_)) :
		  custom_object_type::get(node->attr("type"))),
	base_type_(type_),
    frame_(&type_->default_frame()),
	frame_name_(wml::get_str(node, "current_frame", "normal")),
	time_in_frame_(wml::get_int(node, "time_in_frame")),
	time_in_frame_delta_(wml::get_int(node, "time_in_frame_delta", 1)),
	velocity_x_(wml::get_int(node, "velocity_x")),
	velocity_y_(wml::get_int(node, "velocity_y")),
	accel_x_(wml::get_int(node, "accel_x")),
	accel_y_(wml::get_int(node, "accel_y")),
	gravity_shift_(wml::get_int(node, "gravity_shift", 0)),
	rotate_(static_cast<int64_t>(0)), zorder_(wml::get_int(node, "zorder", type_->zorder())),
	zsub_order_(wml::get_int(node, "zsub_order", type_->zsub_order())),
	hitpoints_(wml::get_int(node, "hitpoints", type_->hitpoints())),
	max_hitpoints_(wml::get_int(node, "max_hitpoints", type_->hitpoints())),
	was_underwater_(false),
	has_feet_(wml::get_bool(node, "has_feet", type_->has_feet())),
	invincible_(0),
	sound_volume_(128),
	vars_(new game_logic::formula_variable_storage(type_->variables())),
	tmp_vars_(new game_logic::formula_variable_storage(type_->tmp_variables())),
	last_hit_by_anim_(0),
	current_animation_id_(0),
	cycle_(wml::get_int(node, "cycle")),
	loaded_(false),
	standing_on_prev_x_(INT_MIN), standing_on_prev_y_(INT_MIN),
	can_interact_with_(false), fall_through_platforms_(0),
	fragment_shaders_(util::split(node->attr("fragment_shaders"))),
	vertex_shaders_(util::split(node->attr("vertex_shaders"))),
	shader_(0),
	always_active_(wml::get_bool(node, "always_active", false)),
	activation_border_(wml::get_int(node, "activation_border", type_->activation_border())),
	last_cycle_active_(0),
	parent_pivot_(node->attr("pivot")),
	parent_prev_x_(0), parent_prev_y_(0), parent_prev_facing_(true),
	min_difficulty_(wml::get_int(node, "min_difficulty", -1)),
	max_difficulty_(wml::get_int(node, "max_difficulty", -1))
{
	if(node->has_attr("platform_area")) {
		set_platform_area(rect(node->attr("platform_area")));
	}

	if(node->has_attr("x_schedule")) {
		if(position_schedule_.get() == NULL) {
			position_schedule_.reset(new position_schedule);
		}

		const std::string& s = node->attr("x_schedule").str();

		int nints = std::count(s.begin(), s.end(), ',')+1;
		position_schedule_->x_pos.resize(nints);
		util::split_into_ints(s.c_str(), &position_schedule_->x_pos[0], &nints);
		position_schedule_->x_pos.resize(nints);
	}

	if(node->has_attr("y_schedule")) {
		if(position_schedule_.get() == NULL) {
			position_schedule_.reset(new position_schedule);
		}

		const std::string& s = node->attr("y_schedule").str();

		int nints = std::count(s.begin(), s.end(), ',')+1;
		position_schedule_->y_pos.resize(nints);
		util::split_into_ints(s.c_str(), &position_schedule_->y_pos[0], &nints);
		position_schedule_->y_pos.resize(nints);
	}

	if(node->has_attr("rotation_schedule")) {
		if(position_schedule_.get() == NULL) {
			position_schedule_.reset(new position_schedule);
		}

		const std::string& s = node->attr("rotation_schedule").str();

		int nints = std::count(s.begin(), s.end(), ',')+1;
		std::vector<int> rotation_ints(nints);
		util::split_into_ints(s.c_str(), &rotation_ints[0], &nints);
		position_schedule_->rotation.resize(nints);
		rotation_ints.resize(nints);
		for(int n = 0; n != nints; ++n) {
			position_schedule_->rotation[n] = decimal(static_cast<int64_t>(rotation_ints[n]));
		}

	}

	if(position_schedule_.get() != NULL && node->has_attr("schedule_speed")) {
		position_schedule_->speed = wml::get_int(node, "schedule_speed");
	}

	if(node->has_attr("draw_area")) {
		draw_area_.reset(new rect(node->attr("draw_area")));
	}

	if(node->has_attr("draw_scale")) {
		draw_scale_.reset(new decimal(static_cast<int64_t>(wml::get_int(node, "draw_scale"))));
	}

	if(node->has_attr("activation_area")) {
		activation_area_.reset(new rect(node->attr("activation_area")));
	}

	if(node->has_attr("clip_area")) {
		clip_area_.reset(new rect(node->attr("clip_area")));
	}

	if(node->has_attr("variations")) {
		current_variation_ = util::split(node->attr("variations"));
		type_ = base_type_->get_variation(current_variation_);
	}

	if(node->has_attr("parallax_scale_x") || node->has_attr("parallax_scale_y")) {
		parallax_scale_millis_.reset(new std::pair<int, int>(wml::get_int(node, "parallax_scale_x", type_->parallax_scale_millis_x()), wml::get_int(node, "parallax_scale_y", type_->parallax_scale_millis_y())));
	} else {
		parallax_scale_millis_.reset(new std::pair<int, int>(type_->parallax_scale_millis_x(), type_->parallax_scale_millis_y()));
	}

	vars_->read(node->get_child("vars"));

	unsigned int solid_dim = type_->solid_dimensions();
	unsigned int weak_solid_dim = type_->weak_solid_dimensions();
	unsigned int collide_dim = type_->collide_dimensions();
	unsigned int weak_collide_dim = type_->weak_collide_dimensions();

	if(node->has_attr("solid_dim")) {
		weak_solid_dim = solid_dim = 0;
		std::vector<std::string> solid_dim_str = util::split(node->attr("solid_dim"));
		foreach(const std::string& str, solid_dim_str) {
			if(str.empty() || str == "level_only") {
				continue;
			}

			if(str[0] == '~') {
				const int id = get_solid_dimension_id(std::string(str.begin() + 1, str.end()));
				weak_solid_dim |= 1 << id;
			} else {
				const int id = get_solid_dimension_id(str);
				solid_dim |= 1 << id;
			}
		}
	}

	if(node->has_attr("collide_dim")) {
		weak_collide_dim = collide_dim = 0;
		std::vector<std::string> collide_dim_str = util::split(node->attr("collide_dim"));
		foreach(const std::string& str, collide_dim_str) {
			if(str.empty() || str == "level_only") {
				continue;
			}

			if(str[0] == '~') {
				const int id = get_solid_dimension_id(std::string(str.begin() + 1, str.end()));
				weak_collide_dim |= 1 << id;
			} else {
				const int id = get_solid_dimension_id(str);
				collide_dim |= 1 << id;
			}
		}
	}

	set_solid_dimensions(solid_dim, weak_solid_dim);
	set_collide_dimensions(collide_dim, weak_collide_dim);

	wml::const_node_ptr tags_node = node->get_child("tags");
	if(tags_node) {
		tags_ = new game_logic::map_formula_callable(node->get_child("tags"));
	} else {
		tags_ = new game_logic::map_formula_callable(type_->tags());
	}

	if(node->has_attr("draw_color")) {
		draw_color_.reset(new graphics::color_transform(node->attr("draw_color")));
	}

	if(node->has_attr("label")) {
		set_label(node->attr("label"));
	} else {
		set_distinct_label();
	}

	if(!type_->respawns()) {
		set_respawn(false);
	}

	assert(type_.get());
	//set_frame_no_adjustments(frame_name_);
	frame_ = &type_->get_frame(frame_name_);
	calculate_solid_rect();

	next_animation_formula_ = type_->next_animation_formula();

	custom_object_type::init_event_handlers(node, event_handlers_);

	can_interact_with_ = get_event_handler(OBJECT_EVENT_INTERACT).get() != NULL;

	wml::const_node_ptr text_node = node->get_child("text");
	if(text_node) {
		set_text(text_node->attr("text"), text_node->attr("font"), wml::get_int(text_node, "size", 2), wml::get_bool(text_node, "centered", false));
	}

	if(node->has_attr("particles")) {
		std::vector<std::string> particles = util::split(node->attr("particles"));
		foreach(const std::string& p, particles) {
			add_particle_system(p, p);
		}
	}

	FOREACH_WML_CHILD(light_node, node, "light") {
		light_ptr new_light(light::create_light(*this, light_node));
		if(new_light) {
			lights_.push_back(new_light);
		}
	}

	if(node->has_attr("parent")) {
		parent_loading_.serialize_from_string(node->attr("parent"));
	}

	if(node->has_attr("platform_offsets")) {
		platform_offsets_ = wml::get_vector_int(node, "platform_offsets");
	} else {
		platform_offsets_ = type_->platform_offsets();
	}
	
	//fprintf(stderr, "object address= %p, ", this);
	//fprintf(stderr, "zsub_order=%d,", zsub_order_);
}

custom_object::custom_object(const std::string& type, int x, int y, bool face_right)
  : entity(x, y, face_right),
    previous_y_(y),
    type_(custom_object_type::get_or_die(type)),
	base_type_(type_),
	frame_(&type_->default_frame()),
    frame_name_("normal"),
	time_in_frame_(0), time_in_frame_delta_(1),
	velocity_x_(0), velocity_y_(0),
	accel_x_(0), accel_y_(0), gravity_shift_(0),
	rotate_(static_cast<int64_t>(0)), zorder_(type_->zorder()),
	zsub_order_(type_->zsub_order()),
	hitpoints_(type_->hitpoints()),
	max_hitpoints_(type_->hitpoints()),
	was_underwater_(false),
	has_feet_(type_->has_feet()),
	invincible_(0),
	sound_volume_(128),
	vars_(new game_logic::formula_variable_storage(type_->variables())),
	tmp_vars_(new game_logic::formula_variable_storage(type_->tmp_variables())),
	tags_(new game_logic::map_formula_callable(type_->tags())),
	last_hit_by_anim_(0),
	cycle_(0),
	loaded_(false), fall_through_platforms_(0),
	shader_(0),
	always_active_(false),
	activation_border_(type_->activation_border()),
	last_cycle_active_(0),
	parent_prev_x_(0), parent_prev_y_(0), parent_prev_facing_(true),
	min_difficulty_(-1), max_difficulty_(-1)
{
	set_solid_dimensions(type_->solid_dimensions(),
	                     type_->weak_solid_dimensions());
	set_collide_dimensions(type_->collide_dimensions(),
	                       type_->weak_collide_dimensions());

	{
		//generate a random label for the object
		char buf[64];
		sprintf(buf, "_%x", rand());
		set_label(buf);
	}

	parallax_scale_millis_.reset(new std::pair<int, int>(type_->parallax_scale_millis_x(), type_->parallax_scale_millis_y()));

	assert(type_.get());
	set_frame_no_adjustments(frame_name_);

	next_animation_formula_ = type_->next_animation_formula();
}

custom_object::custom_object(const custom_object& o) :
	entity(o),
	previous_y_(o.previous_y_),
	custom_type_(o.custom_type_),
	type_(o.type_),
	base_type_(o.base_type_),
	current_variation_(o.current_variation_),
	frame_(o.frame_),
	frame_name_(o.frame_name_),
	time_in_frame_(o.time_in_frame_),
	time_in_frame_delta_(o.time_in_frame_delta_),
	velocity_x_(o.velocity_x_), velocity_y_(o.velocity_y_),
	accel_x_(o.accel_x_), accel_y_(o.accel_y_),
	gravity_shift_(o.gravity_shift_),
	rotate_(o.rotate_),
	zorder_(o.zorder_),
	zsub_order_(o.zsub_order_),
	parallax_scale_millis_(new std::pair<int, int>(*o.parallax_scale_millis_)),
	hitpoints_(o.hitpoints_),
	max_hitpoints_(o.max_hitpoints_),
	was_underwater_(o.was_underwater_),
	has_feet_(o.has_feet_),
	invincible_(o.invincible_),
	sound_volume_(o.sound_volume_),
	next_animation_formula_(o.next_animation_formula_),

	vars_(new game_logic::formula_variable_storage(*o.vars_)),
	tmp_vars_(new game_logic::formula_variable_storage(*o.tmp_vars_)),
	tags_(new game_logic::map_formula_callable(*o.tags_)),

	last_hit_by_(o.last_hit_by_),
	last_hit_by_anim_(o.last_hit_by_anim_),
	current_animation_id_(o.current_animation_id_),
	cycle_(o.cycle_),
	loaded_(o.loaded_),
	event_handlers_(o.event_handlers_),
	standing_on_(o.standing_on_),
	standing_on_prev_x_(o.standing_on_prev_x_), standing_on_prev_y_(o.standing_on_prev_y_),
	distortion_(o.distortion_),
	draw_color_(o.draw_color_ ? new graphics::color_transform(*o.draw_color_) : NULL),
	draw_scale_(o.draw_scale_ ? new decimal(*o.draw_scale_) : NULL),
	draw_area_(o.draw_area_ ? new rect(*o.draw_area_) : NULL),
	activation_area_(o.activation_area_ ? new rect(*o.activation_area_) : NULL),
	clip_area_(o.clip_area_ ? new rect(*o.clip_area_) : NULL),
	can_interact_with_(o.can_interact_with_),
	particle_systems_(o.particle_systems_),
	text_(o.text_),
	driver_(o.driver_),
	blur_(o.blur_),
	fall_through_platforms_(o.fall_through_platforms_),
	fragment_shaders_(o.fragment_shaders_),
	vertex_shaders_(o.vertex_shaders_),
	shader_(o.shader_),
	always_active_(o.always_active_),
	activation_border_(o.activation_border_),
	last_cycle_active_(0),
	parent_(o.parent_),
	parent_pivot_(o.parent_pivot_),
	parent_prev_x_(o.parent_prev_x_),
	parent_prev_y_(o.parent_prev_y_),
	parent_prev_facing_(o.parent_prev_facing_),
	min_difficulty_(o.min_difficulty_),
	max_difficulty_(o.max_difficulty_),
	custom_draw_(o.custom_draw_),
	platform_offsets_(o.platform_offsets_)
{
}

custom_object::~custom_object()
{
	sound::stop_looped_sounds(this);
}

void custom_object::finish_loading()
{
	if(parent_loading_.is_null() == false) {
		entity_ptr p = parent_loading_.try_convert<entity>();
		if(p) {
			parent_ = p;
		}
		parent_loading_ = variant();
	}
}

bool custom_object::serializable() const
{
	return type_->serializable();
}

wml::node_ptr custom_object::write() const
{
	wml::node_ptr res(new wml::node("character"));
	if(parallax_scale_millis_.get() != NULL) {
		if( (type_->parallax_scale_millis_x() !=  parallax_scale_millis_->first) || (type_->parallax_scale_millis_y() !=  parallax_scale_millis_->second)){
			res->set_attr("parallax_scale_x", formatter() << parallax_scale_millis_->first);
			res->set_attr("parallax_scale_y", formatter() << parallax_scale_millis_->second);
		}
	}

	if(platform_area_.get() != NULL) {
		res->set_attr("platform_area", platform_area_->to_string());
	}

	if(always_active_) {
		res->set_attr("always_active", "yes");
	}

	if(activation_border_ != type_->activation_border()) {
		res->set_attr("activation_border", formatter() << activation_border_);
	}
	
	if(position_schedule_.get() != NULL) {
		res->set_attr("schedule_speed", formatter() << position_schedule_->speed);
		if(position_schedule_->x_pos.empty() == false) {
			res->set_attr("x_schedule", util::join_ints(&position_schedule_->x_pos[0], position_schedule_->x_pos.size()));
		}

		if(position_schedule_->y_pos.empty() == false) {
			res->set_attr("y_schedule", util::join_ints(&position_schedule_->y_pos[0], position_schedule_->y_pos.size()));
		}

		if(position_schedule_->rotation.empty() == false) {
			std::vector<int> v(position_schedule_->rotation.size());
			for(int n = 0; n != v.size(); ++n) {
				v[n] = position_schedule_->rotation[n].value();
			}

			res->set_attr("rotation_schedule", util::join_ints(&v[0], v.size()));
		}
	}

	if(!attached_objects().empty()) {
		std::string s;

		foreach(const entity_ptr& e, attached_objects()) {
			if(s.empty() == false) {
				s += ",";
			}

			char buf[256];
			sprintf(buf, "%p", e.get());
			s += buf;
		}

		res->set_attr("attached_objects", s);
	}

	if(!current_variation_.empty()) {
		res->set_attr("variations", util::join(current_variation_));
	}

	if(draw_color_) {
		res->set_attr("draw_color", draw_color_->to_string());
	}

	if(label().empty() == false) {
		res->set_attr("label", label());
	}

	if(cycle_ > 1) {
		res->set_attr("cycle", formatter() << cycle_);
	}

	if(frame_name_ != "default") {
		res->set_attr("current_frame", frame_name_);
	}

	res->set_attr("custom", "yes");
	res->set_attr("type", type_->id());
	res->set_attr("x", formatter() << x());
	res->set_attr("y", formatter() << y());
	res->set_attr("velocity_x", formatter() << velocity_x_);
	res->set_attr("velocity_y", formatter() << velocity_y_);
	
	if(platform_motion_x()) {
		res->set_attr("platform_motion_x", formatter() << platform_motion_x());
	}

	if(solid_dimensions() != type_->solid_dimensions() ||
	   weak_solid_dimensions() != type_->weak_solid_dimensions()) {
		std::string solid_dim;
		for(int n = 0; n != 32; ++n) {
			if(solid_dimensions()&(1 << n)) {
				if(!solid_dim.empty()) {
					solid_dim += ",";
				}

				solid_dim += get_solid_dimension_key(n);
			}

			if(weak_solid_dimensions()&(1 << n)) {
				if(!solid_dim.empty()) {
					solid_dim += ",";
				}

				solid_dim += "~" + get_solid_dimension_key(n);
			}
		}

		if(solid_dim.empty()) {
			solid_dim = "level_only";
		}

		res->set_attr("solid_dim", solid_dim);
	}

	if(collide_dimensions() != type_->collide_dimensions() ||
	   weak_collide_dimensions() != type_->weak_collide_dimensions()) {
		std::string collide_dim, weak_collide_dim;
		for(int n = 0; n != 32; ++n) {
			if(collide_dimensions()&(1 << n)) {
				if(!collide_dim.empty()) {
					collide_dim += ",";
				}

				collide_dim += get_solid_dimension_key(n);
			}

			if(weak_collide_dimensions()&(1 << n)) {
				if(!collide_dim.empty()) {
					collide_dim += ",";
				}

				collide_dim += "~" + get_solid_dimension_key(n);
			}
		}

		if(collide_dim.empty()) {
			collide_dim = "level_only";
		}

		res->set_attr("collide_dim", collide_dim);
	}

	if(hitpoints_ != type_->hitpoints() || max_hitpoints_ != type_->hitpoints()) {
		res->set_attr("hitpoints", formatter() << hitpoints_);
		res->set_attr("max_hitpoints", formatter() << max_hitpoints_);
	}

	if(!vertex_shaders_.empty()) {
		res->set_attr("vertex_shaders", util::join(vertex_shaders_));
	}

	if(!fragment_shaders_.empty()) {
		res->set_attr("fragment_shaders", util::join(fragment_shaders_));
	}

	if(zorder_ != type_->zorder()) {
		res->set_attr("zorder", formatter() << zorder_);
	}

	if(parallax_scale_millis_.get()) {
		if(parallax_scale_millis_->first != type_->parallax_scale_millis_x() || parallax_scale_millis_->second != type_->parallax_scale_millis_y()){
			res->set_attr("parallax_scale_x", formatter() << parallax_scale_millis_->first);
			res->set_attr("parallax_scale_y", formatter() << parallax_scale_millis_->second);
		}
	}
	   
	
	
	if(zsub_order_ != 0) {
		res->set_attr("zsub_order", formatter() << zsub_order_);
	}
	
	res->set_attr("face_right", face_right() ? "yes" : "no");
	if(upside_down()) {
		res->set_attr("upside_down", "yes");
	}

	res->set_attr("time_in_frame", formatter() << time_in_frame_);

	if(time_in_frame_delta_ != 1) {
		res->set_attr("time_in_frame_delta", formatter() << time_in_frame_delta_);
	}

	if(has_feet_ != type_->has_feet()) {
		res->set_attr("has_feet", formatter() << (has_feet_ ? "yes" : "no"));
	}

	if(group() >= 0) {
		res->set_attr("group", formatter() << group());
	}

	for(int n = 0; n != event_handlers_.size(); ++n) {
		if(!event_handlers_[n]) {
			continue;
		}

		res->set_attr("on_" + get_object_event_str(n), event_handlers_[n]->str());
	}

	if(!vars_->equal_to(type_->variables())) {
		wml::node_ptr vars(new wml::node("vars"));
		vars_->write(vars);
		res->add_child(vars);
	}

	if(tags_->values() != type_->tags()) {
		wml::node_ptr tags(new wml::node("tags"));
		tags_->write(tags);
		res->add_child(tags);
	}

	if(custom_type_) {
		res->add_child(wml::deep_copy(custom_type_));
	}

	if(text_) {
		wml::node_ptr node(new wml::node("text"));
		node->set_attr("text", text_->text);
		if(text_->font) {
			node->set_attr("font", text_->font->id());
		}

		node->set_attr("size", formatter() << text_->size);

		res->add_child(node);
	}

	if(draw_area_) {
		res->set_attr("draw_area", draw_area_->to_string());
	}

	if(draw_scale_) {
		res->set_attr("draw_scale", formatter() << draw_scale_->value());
	}

	if(activation_area_) {
		res->set_attr("activation_area", activation_area_->to_string());
	}

	if(clip_area_) {
		res->set_attr("clip_area", clip_area_->to_string());
	}

	if(!particle_systems_.empty()) {
		std::string systems;
		for(std::map<std::string, particle_system_ptr>::const_iterator i = particle_systems_.begin(); i != particle_systems_.end(); ++i) {
			if(i->second->should_save() == false) {
				continue;
			}

			if(!systems.empty()) {
				systems += ",";
			}

			systems += i->first;
		}

		if(!systems.empty()) {
			res->set_attr("particles", systems);
		}
	}

	foreach(const light_ptr& p, lights_) {
		res->add_child(p->write());
	}

	if(parent_.get() != NULL) {
		std::string str;
		variant(parent_.get()).serialize_to_string(str);
		res->set_attr("parent", str);
	}

	if(parent_pivot_.empty() == false) {
		res->set_attr("pivot", parent_pivot_);
	}

	if(min_difficulty_ != -1) {
		res->set_attr("min_difficulty", formatter() << min_difficulty_);
	}

	if(max_difficulty_ != -1) {
		res->set_attr("max_difficulty", formatter() << max_difficulty_);
	}

	if(platform_offsets_.empty() == false) {
		res->set_attr("platform_offsets", util::join_ints(&platform_offsets_.front(), platform_offsets_.size()));
	}
	
	return res;
}

void custom_object::setup_drawing() const
{
	if(distortion_) {
		graphics::add_raster_distortion(distortion_.get());
	}
}

void custom_object::draw() const
{
	if(frame_ == NULL) {
		return;
	}

	if(shader_ == 0 && !fragment_shaders_.empty() && !vertex_shaders_.empty()) {
		shader_ = get_gl_shader(vertex_shaders_, fragment_shaders_);
	}

#ifndef SDL_VIDEO_OPENGL_ES
	if(shader_) {
		glUseProgram(shader_);

		if(shader_vars_) {
			for(game_logic::map_formula_callable::const_iterator i = shader_vars_->begin(); i != shader_vars_->end(); ++i) {
				GLuint id = glGetUniformLocation(shader_, i->first.c_str());
				glUniform1f(id, i->second.as_int()/1000.0);
			}
		}
	}

	if(clip_area_) {
		graphics::push_clip(clip_area_->sdl_rect());
	}
#endif

	if(driver_) {
		driver_->draw();
	}

	foreach(const entity_ptr& attached, attached_objects()) {
		attached->draw();
	}

	if(draw_color_) {
		draw_color_->to_color().set_as_current_color();
	}

	const int draw_x = x();
	const int draw_y = y();

	if(type_->hidden_in_game() && !level::current().in_editor()) {
		//pass
	} else if(custom_draw_.get() != NULL) {
		frame_->draw_custom(draw_x-draw_x%2, draw_y-draw_y%2, *custom_draw_, draw_area_.get(), face_right(), upside_down(), time_in_frame_, rotate_.as_float());
	} else if(draw_scale_) {
		frame_->draw(draw_x-draw_x%2, draw_y-draw_y%2, face_right(), upside_down(), time_in_frame_, rotate_.as_float(), draw_scale_->as_float());
	} else if(!draw_area_.get()) {
		frame_->draw(draw_x-draw_x%2, draw_y-draw_y%2, face_right(), upside_down(), time_in_frame_, rotate_.as_float());
	} else {
		frame_->draw(draw_x-draw_x%2, draw_y-draw_y%2, *draw_area_, face_right(), upside_down(), time_in_frame_, rotate_.as_float());
	}

	if(blur_) {
		blur_->draw();
	}

	if(draw_color_) {
		if(!draw_color_->fits_in_color()) {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			graphics::color_transform transform = *draw_color_;
			while(!transform.fits_in_color()) {
				transform = transform - transform.to_color();
				transform.to_color().set_as_current_color();
				frame_->draw(draw_x-draw_x%2, draw_y-draw_y%2, face_right(), upside_down(), time_in_frame_, rotate_.as_float());
			}

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		glColor4ub(255, 255, 255, 255);
	}

//	if(draw_color_int_ != DefaultColor) {
//		glColor4ub(255, 255, 255, 255);
//	}

	draw_debug_rects();

	for(std::map<std::string, particle_system_ptr>::const_iterator i = particle_systems_.begin(); i != particle_systems_.end(); ++i) {
		i->second->draw(rect(last_draw_position().x/100, last_draw_position().y/100, graphics::screen_width(), graphics::screen_height()), *this);
	}

	if(text_ && text_->font && text_->alpha) {
		glColor4ub(255, 255, 255, text_->alpha);
		if (text_->centered)
			text_->font->draw(draw_x-text_->dimensions.w()/2, draw_y, text_->text, text_->size);
		else
			text_->font->draw(draw_x, draw_y, text_->text, text_->size);
		glColor4ub(255, 255, 255, 255);
	}
	
#ifndef SDL_VIDEO_OPENGL_ES
	if(clip_area_) {
		graphics::pop_clip();
	}

	if(shader_) {
		glUseProgram(0);
	}
#endif
}

void custom_object::draw_group() const
{
	if(label().empty() == false && label()[0] != '_') {
		blit_texture(font::render_text(label(), graphics::color_yellow(), 32), x(), y() + 26);
	}

	if(group() >= 0) {
		blit_texture(font::render_text(formatter() << group(), graphics::color_yellow(), 24), x(), y());
	}
}

void custom_object::process(level& lvl)
{
	if(type_->use_image_for_collisions()) {
		//anything that uses their image for collisions is a static,
		//un-moving object that will stay immobile.
		return;
	}

	if(lvl.in_editor()) {
		if(!type_->static_object() && entity_collides(level::current(), *this, MOVE_NONE)) {
			//The object collides illegally, but we're in the editor. Freeze
			//the object by returning, since we can't process it.
			return;
		}
	}

	collision_info debug_collide_info;
	ASSERT_LOG(type_->static_object() || lvl.in_editor() || !entity_collides(level::current(), *this, MOVE_NONE, &debug_collide_info), "ENTITY " << debug_description() << " COLLIDES WITH " << (debug_collide_info.collide_with ? debug_collide_info.collide_with->debug_description() : "THE LEVEL") << " AT START OF PROCESS");

	if(parent_.get() != NULL) {
		const point pos = parent_position();
		const int move_x = pos.x - parent_prev_x_;
		const int move_y = pos.y - parent_prev_y_;

		move_centipixels(move_x*100, move_y*100);

		const bool parent_facing = parent_->face_right();
		if(parent_facing != parent_prev_facing_) {
			const point pos_before_turn = parent_->pivot(parent_pivot_);

			const int relative_x = pos.x - pos_before_turn.x;

			move_centipixels(relative_x*100, 0);
		}

		parent_prev_x_ = pos.x;
		parent_prev_y_ = pos.y;
		parent_prev_facing_ = parent_facing;
	}

	if(last_cycle_active_ < lvl.cycle() - 5) {
		handle_event(OBJECT_EVENT_BECOME_ACTIVE);
	}

	last_cycle_active_ = lvl.cycle();

	entity::process(lvl);

	//the object should never be colliding with the level at the start of processing.
//	assert(!entity_collides_with_level(lvl, *this, MOVE_NONE));
//	assert(!entity_collides(lvl, *this, MOVE_NONE));

	//this is a flag which tracks whether we've fired a collide_feet
	//event. If we don't fire a collide_feet event through normal collision
	//detection, but we change the object we're standing on, we should
	//still fire a collide_feet event.
	bool fired_collide_feet = false;

	collision_info stand_info;
	const bool started_standing = is_standing(lvl, &stand_info);
	if(!started_standing && standing_on_) {
		//if we were standing on something the previous frame, but aren't
		//standing any longer, we use the value of what we were previously
		//standing on.
		stand_info.traction = standing_on_->surface_traction();
		stand_info.friction = standing_on_->surface_friction();
	}

	if(y() > lvl.boundaries().y2()) {
		--hitpoints_;
	}
	
	previous_y_ = y();
	if(started_standing && velocity_y_ > 0) {
		velocity_y_ = 0;
	}

	const int start_x = x();
	const int start_y = y();
	const decimal start_rotate = rotate_;
	++cycle_;

	if(invincible_) {
		--invincible_;
	}

	if(!loaded_) {
		handle_event(OBJECT_EVENT_LOAD);
		loaded_ = true;
	}

	if(cycle_ == 1) {
		handle_event(OBJECT_EVENT_CREATE);
		handle_event(OBJECT_EVENT_DONE_CREATE);
	}

	variant scheduled_command = get_scheduled_command(lvl.cycle());
	while(!scheduled_command.is_null()) {
		execute_command(scheduled_command);
		scheduled_command = get_scheduled_command(lvl.cycle());
	}

	if(position_schedule_.get() != NULL) {
		const int pos = cycle_/position_schedule_->speed;
		const int next_fraction = cycle_%position_schedule_->speed;
		const int this_fraction = position_schedule_->speed - next_fraction;

		int xpos = INT_MIN, ypos = INT_MIN;
		if(position_schedule_->x_pos.empty() == false) {
			xpos = position_schedule_->x_pos[pos%position_schedule_->x_pos.size()];
			if(next_fraction && pos+1 != position_schedule_->x_pos.size()) {
				xpos = (xpos*this_fraction + next_fraction*position_schedule_->x_pos[(pos+1)%position_schedule_->x_pos.size()])/position_schedule_->speed;
			}
		}

		if(position_schedule_->y_pos.empty() == false) {
			ypos = position_schedule_->y_pos[pos%position_schedule_->y_pos.size()];
			if(next_fraction && pos+1 != position_schedule_->y_pos.size()) {
				ypos = (ypos*this_fraction + next_fraction*position_schedule_->y_pos[(pos+1)%position_schedule_->y_pos.size()])/position_schedule_->speed;
			}
		}

		if(xpos != INT_MIN && ypos != INT_MIN) {
			set_pos(xpos, ypos);
		} else if(xpos != INT_MIN) {
			set_x(xpos);
		} else if(ypos != INT_MIN) {
			set_y(ypos);
		}

		if(position_schedule_->rotation.empty() == false) {
			rotate_ = position_schedule_->rotation[pos%position_schedule_->rotation.size()];
			while(rotate_ >= 360) {
				rotate_ -= 360;
			}

			if(next_fraction) {
				rotate_ = decimal((rotate_*this_fraction + next_fraction*position_schedule_->rotation[(pos+1)%position_schedule_->rotation.size()])/position_schedule_->speed);
			}
		}
	}

	if(stand_info.damage) {
		handle_event(OBJECT_EVENT_SURFACE_DAMAGE);
	}

	time_in_frame_ += time_in_frame_delta_;
	if(time_in_frame_ < 0) {
		time_in_frame_ = 0;
	}

	if(time_in_frame_ > frame_->duration()) {
		time_in_frame_ = frame_->duration();
	}

	if(time_in_frame_ == frame_->duration()) {
		handle_event(frame_->end_event_id());
		handle_event(OBJECT_EVENT_END_ANIM);
		if(next_animation_formula_) {
			variant var = next_animation_formula_->execute(*this);
			set_frame(var.as_string());
		}
	}

	const std::string* event = frame_->get_event(time_in_frame_);
	if(event) {
		handle_event(*event);
	}

	const bool is_underwater = solid() && lvl.is_underwater(solid_rect());
	
	if( is_underwater && !was_underwater_){
		//event on_enter_water
		handle_event(OBJECT_EVENT_ENTER_WATER);
		was_underwater_ = true;
	}else if ( !is_underwater && was_underwater_ ){
		//event on_exit_water
		handle_event(OBJECT_EVENT_EXIT_WATER);
		was_underwater_ = false;
	}
	
	if(type_->static_object()) {
		static_process(lvl);
		return;
	}

	const int traction_from_surface = (stand_info.traction*type_->traction())/1000;
	velocity_x_ += (accel_x_ * (stand_info.traction ? traction_from_surface : (is_underwater?type_->traction_in_water() :type_->traction_in_air())) * (face_right() ? 1 : -1))/1000;
	if(!standing_on_ && !started_standing || accel_y_ < 0) {
		//do not accelerate downwards if standing on something.
		velocity_y_ += accel_y_ * (gravity_shift_ + (is_underwater ? type_->traction_in_water() : 1000))/1000;
	}

	if(type_->friction()) {

		const int air_resistance = is_underwater ? lvl.water_resistance() : lvl.air_resistance();

		const int friction = ((stand_info.friction + air_resistance)*type_->friction())/1000;
		int vertical_resistance = (air_resistance*type_->friction())/1000;
		if(velocity_y_ > 0 && !is_underwater) {
			//vertical air resistance is reduced when moving downwards.
			//This works well for most objects, though consider making it
			//configurable in future.
			vertical_resistance /= 2;
		}

		velocity_x_ = (velocity_x_*(1000 - friction))/1000;
		velocity_y_ = (velocity_y_*(1000 - vertical_resistance))/1000;
	}

	if(type_->affected_by_currents()) {
		lvl.get_current(*this, &velocity_x_, &velocity_y_);
	}

	bool collide = false;

	//calculate velocity which takes into account velocity of the object we're standing on.
	int effective_velocity_x = velocity_x_;
	int effective_velocity_y = velocity_y_;

	if(effective_velocity_y > 0 && (standing_on_ || started_standing)) {
		effective_velocity_y = 0;
	}

	int platform_motion_x_movement = 0;
	if(standing_on_) {

		platform_motion_x_movement = standing_on_->platform_motion_x() + standing_on_->map_platform_pos(feet_x())*100;
		effective_velocity_x += (standing_on_->feet_x() - standing_on_prev_x_)*100 + platform_motion_x_movement;
		effective_velocity_y += (standing_on_->feet_y() - standing_on_prev_y_)*100;
	}

	if(stand_info.collide_with != standing_on_ && stand_info.adjust_y) {
		//if we're landing on a new platform, we might have to adjust our
		//y position to suit its last movement and put us on top of
		//the platform.

		effective_velocity_y -= stand_info.adjust_y*100;
	}

	if(effective_velocity_x || effective_velocity_y) {
		if(!solid() && !type_->object_level_collisions()) {
			move_centipixels(effective_velocity_x, effective_velocity_y);
			effective_velocity_x = 0;
			effective_velocity_y = 0;
		} else if(!has_feet() && solid()) {
			move_centipixels(effective_velocity_x, effective_velocity_y);
			if(is_flightpath_clear(lvl, *this, solid_rect())) {
				effective_velocity_x = 0;
				effective_velocity_y = 0;
			} else {
				//we can't guarantee smooth movement to this location, so
				//roll the move back and we'll do a pixel-by-pixel move
				//until we collide.
				move_centipixels(-effective_velocity_x, -effective_velocity_y);
			}
		}
	}


	collision_info collide_info;
	collision_info jump_on_info;

	bool is_stuck = false;

	collide = false;
	int move_left;
	for(move_left = std::abs(effective_velocity_y); move_left > 0 && !collide && !type_->ignore_collide(); move_left -= 100) {
		const int dir = effective_velocity_y > 0 ? 1 : -1;
		int damage = 0;

		const int original_centi_y = centi_y();

		const int move_amount = std::min(std::max(move_left, 0), 100);
		
		const bool moved = move_centipixels(0, move_amount*dir);
		if(!moved) {
			//we didn't actually move any pixels, so just abort.
			break;
		}

		if(type_->object_level_collisions() && non_solid_entity_collides_with_level(lvl, *this)) {
			handle_event(OBJECT_EVENT_COLLIDE_LEVEL);
		}

		if(effective_velocity_y > 0) {
			if(entity_collides(lvl, *this, MOVE_DOWN, &collide_info)) {
				//our 'legs' but not our feet collide with the level. Try to
				//move one pixel to the left or right and see if either
				//direction makes us no longer colliding.
				set_x(x() + 1);
				if(entity_collides(lvl, *this, MOVE_DOWN) || entity_collides(lvl, *this, MOVE_RIGHT)) {
					set_x(x() - 2);
					if(entity_collides(lvl, *this, MOVE_DOWN) || entity_collides(lvl, *this, MOVE_LEFT)) {
						//moving in either direction fails to resolve the collision.
						//This effectively means the object is 'stuck' in a small
						//pit.
						set_x(x() + 1);
						move_centipixels(0, -move_amount*dir);
						collide = true;
						is_stuck = true;
						break;
					}
				}
				

			}
		} else {
			//effective_velocity_y < 0 -- going up
			if(entity_collides(lvl, *this, MOVE_UP, &collide_info)) {
				collide = true;
				move_centipixels(0, -move_amount*dir);
				break;
			}
		}

		if(!collide && !type_->ignore_collide() && effective_velocity_y > 0 && is_standing(lvl, &jump_on_info)) {
			if(!jump_on_info.collide_with || jump_on_info.collide_with != standing_on_) {
				collide = true;
				collide_info = jump_on_info;
			}

			break;
		}

		if(collide) {
			break;
		}
	}

	//this variable handled whether we already landed in our vertical movement
	//in which case horizontal movement won't consider us to land.
	bool vertical_landed = false;

	if(is_stuck) {
		handle_event(OBJECT_EVENT_STUCK);
	}

	if(collide) {
		if(effective_velocity_y > 0) {
			vertical_landed = true;
		}

		if(effective_velocity_y < 0 || !started_standing) {

			game_logic::map_formula_callable* callable = new game_logic::map_formula_callable;
			variant v(callable);
	
			if(collide_info.area_id != NULL) {
				callable->add("area", variant(*collide_info.area_id));
			}

			if(collide_info.collide_with) {
				callable->add("collide_with", variant(collide_info.collide_with.get()));
				if(collide_info.collide_with_area_id) {
					callable->add("collide_with_area", variant(*collide_info.collide_with_area_id));
				}

			}

			handle_event(effective_velocity_y < 0 ? OBJECT_EVENT_COLLIDE_HEAD : OBJECT_EVENT_COLLIDE_FEET, callable);
			fired_collide_feet = true;
		}

		if(collide_info.damage || jump_on_info.damage) {
			game_logic::map_formula_callable* callable = new game_logic::map_formula_callable;
			callable->add("surface_damage", variant(std::max(collide_info.damage, jump_on_info.damage)));
			variant v(callable);
			handle_event(OBJECT_EVENT_COLLIDE_DAMAGE, callable);
		}
	}

	//If the object started out standing on a platform, keep it doing so.
	if(standing_on_ && !fall_through_platforms_ && velocity_y_ >= 0) {
		const int left_foot = feet_x() - type_->feet_width();
		const int right_foot = feet_x() + type_->feet_width();

		int target_y = INT_MAX;
		rect area = standing_on_->platform_rect();
		if(left_foot >= area.x() && left_foot < area.x() + area.w()) {
			rect area = standing_on_->platform_rect_at(left_foot);
			target_y = area.y();
		}

		if(right_foot >= area.x() && right_foot < area.x() + area.w()) {
			rect area = standing_on_->platform_rect_at(right_foot);
			if(area.y() < target_y) {
				target_y = area.y();
			}
		}

		if(target_y != INT_MAX) {
			const int delta = target_y - feet_y();
			const int dir = delta > 0 ? 1 : -1;
			int nmoves = 0;
			for(int n = 0; n != delta; n += dir) {
				set_y(y()+dir);
				++nmoves;
				if(entity_collides(lvl, *this, dir < 0 ? MOVE_UP : MOVE_DOWN)) {
					set_y(y()-dir);
					break;
				}
			}
		}
	}

	collide = false;

	bool horizontal_landed = false;

	//we go through up to two passes of moving an object horizontally. On the
	//first pass, we are 'optimistic' and move the object along, assuming there
	//will be no collisions. Then at the end of the pass we see if the object is
	//colliding. If it's not, all is good, but if it is, we'll re-do the movement,
	//detecting for collisions at each step, until we work out where exactly
	//the collision occurs, and stop the object there.
	for(int detect_collisions = 0; detect_collisions <= 1 && effective_velocity_x; ++detect_collisions) {
		const int backup_centi_x = centi_x();
		const int backup_centi_y = centi_y();


		for(move_left = std::abs(effective_velocity_x); move_left > 0 && !collide && !type_->ignore_collide(); move_left -= 100) {
			if(type_->object_level_collisions() && non_solid_entity_collides_with_level(lvl, *this)) {
				handle_event(OBJECT_EVENT_COLLIDE_LEVEL);
			}

			const STANDING_STATUS previous_standing = is_standing(lvl);

			const int dir = effective_velocity_x > 0 ? 1 : -1;
			const int original_centi_y = centi_y();

			const int move_amount = std::min(std::max(move_left, 0), 100);
		
			const bool moved = move_centipixels(move_amount*dir, 0);
			if(!moved) {
				//we didn't actually move any pixels, so just abort.
				break;
			}

			const int left_foot = feet_x() - type_->feet_width();
			const int right_foot = feet_x() + type_->feet_width();
			bool place_on_object = false;
			if(standing_on_ && !fall_through_platforms_ && velocity_y_ >= 0) {
				rect area = standing_on_->platform_rect();
				if(left_foot >= area.x() && left_foot < area.x() + area.w() ||
					right_foot >= area.x() && right_foot < area.x() + area.w()) {
					place_on_object = true;
				}
			}

			//if we go up or down a slope, and we began the frame standing,
			//move the character up or down as appropriate to try to keep
			//them standing.

			const STANDING_STATUS standing = is_standing(lvl);
			if(place_on_object) {
				int target_y = INT_MAX;
				rect area = standing_on_->platform_rect();
				if(left_foot >= area.x() && left_foot < area.x() + area.w()) {
					const rect area = standing_on_->platform_rect_at(left_foot);
					target_y = area.y();
				}

				if(right_foot >= area.x() && right_foot < area.x() + area.w()) {
					const rect area = standing_on_->platform_rect_at(right_foot);
					if(area.y() < target_y) {
						target_y = area.y();
					}
				}

				const int delta = target_y - feet_y();
				const int dir = delta > 0 ? 1 : -1;
				for(int n = 0; n != delta; n += dir) {
					set_y(y()+dir);
					if(detect_collisions && entity_collides(lvl, *this, dir < 0 ? MOVE_UP : MOVE_DOWN)) {
						set_y(y()-dir);
						break;
					}
				}
			} else if(previous_standing && standing < previous_standing) {

				//we were standing, but we're not now. We want to look for
				//slopes that will enable us to still be standing. We see
				//if the object is trying to walk down stairs, in which case
				//we look downwards first, otherwise we look upwards first,
				//then downwards.
				int dir = walk_up_or_down_stairs() > 0 ? 1 : -1;

				for(int tries = 0; tries != 2; ++tries) {
					bool resolved = false;
					const int SearchRange = 2;
					for(int n = 0; n != SearchRange; ++n) {
						set_y(y()+dir);
						if(detect_collisions && entity_collides(lvl, *this, dir < 0 ? MOVE_UP : MOVE_DOWN)) {
							break;
						}

						if(is_standing(lvl) >= previous_standing) {
							resolved = true;
							break;
						}
					}

					if(resolved) {
						break;
					}

					dir *= -1;
					set_centi_y(original_centi_y);
				}
			} else if(standing) {
				if(!vertical_landed && !started_standing && !standing_on_) {
					horizontal_landed = true;
				}

				collision_info slope_standing_info;

				bool collide_head = false;

				//we are standing, but we need to see if we should be standing
				//on a higher point. If there are solid points immediately above
				//where we are, we adjust our feet to be on them.
				//
				//However, if there is a platform immediately above us, we only
				//adjust our feet upward if the object is trying to walk up
				//stairs, normally by the player pressing up while walking.
				const int begin_y = feet_y();
				int max_slope = 5;
				while(--max_slope && is_standing(lvl, &slope_standing_info)) {
					if(slope_standing_info.platform && walk_up_or_down_stairs() >= 0) {
						if(max_slope == 4) {
							//we always move at least one pixel up, if there is
							//solid, otherwise we'll fall through.
							set_y(y()-1);
							if(detect_collisions && entity_collides(lvl, *this, MOVE_UP)) {
								collide_head = true;
								break;
							}
						}
						break;
					}
	
					set_y(y()-1);
					if(detect_collisions && entity_collides(lvl, *this, MOVE_UP)) {
						collide_head = true;
						break;
					}
				}
	
				if(!max_slope || collide_head) {
					set_centi_y(original_centi_y);
				} else {
					set_y(y()+1);
				}
	
				if(walk_up_or_down_stairs() > 0) {
					//if we are trying to walk down stairs and we're on a platform
					//and one pixel below is walkable, then we move down by
					//one pixel.
					is_standing(lvl, &slope_standing_info);
					if(slope_standing_info.platform) {
						set_y(y()+1);
						if(!is_standing(lvl) || detect_collisions && entity_collides(lvl, *this, MOVE_DOWN)) {
							set_y(y()-1);
						}
					}
				}
			}

			if(detect_collisions && entity_collides(lvl, *this, centi_y() != original_centi_y ? MOVE_NONE : (dir > 0 ? MOVE_RIGHT : MOVE_LEFT), &collide_info)) {
				collide = true;
			}

			if(collide) {
				//undo the move to cancel out the collision
				move_centipixels(-dir*move_amount, 0);
				set_centi_y(original_centi_y);
				break;
			}
		}

		if(!detect_collisions) {
			if(entity_collides(lvl, *this, MOVE_NONE)) {
				set_centi_x(backup_centi_x);
				set_centi_y(backup_centi_y);
			} else {
				break;
			}
		}
	}

	if(collide || horizontal_landed) {

		game_logic::map_formula_callable* callable = new game_logic::map_formula_callable;
		variant v(callable);

		if(collide_info.area_id != NULL) {
			callable->add("area", variant(*collide_info.area_id));
		}

		if(collide_info.collide_with) {
			callable->add("collide_with", variant(collide_info.collide_with.get()));
			if(collide_info.collide_with_area_id) {
				callable->add("collide_with_area", variant(*collide_info.collide_with_area_id));
			}
		}

		handle_event(collide ? OBJECT_EVENT_COLLIDE_SIDE : OBJECT_EVENT_COLLIDE_FEET, callable);
		fired_collide_feet = true;
		if(collide_info.damage) {
			game_logic::map_formula_callable* callable = new game_logic::map_formula_callable;
			callable->add("surface_damage", variant(collide_info.damage));
			variant v(callable);
			handle_event(OBJECT_EVENT_COLLIDE_DAMAGE, callable);
		}
	}

	stand_info = collision_info();
	if(velocity_y_ >= 0) {
		is_standing(lvl, &stand_info);
	}

	if(stand_info.collide_with && standing_on_ != stand_info.collide_with &&
	   effective_velocity_y < stand_info.collide_with->velocity_y()) {
		stand_info.collide_with = NULL;
	}

	if(standing_on_ && standing_on_ != stand_info.collide_with) {
		//we were previously standing on an object and we're not anymore.
		//add the object we were standing on's velocity to ours
		velocity_x_ += standing_on_->last_move_x()*100 + platform_motion_x_movement;
		velocity_y_ += standing_on_->last_move_y()*100;
	}

	if(stand_info.collide_with && standing_on_ != stand_info.collide_with) {
		if(!fired_collide_feet) {
		}

		//we are standing on a new object. Adjust our velocity relative to
		//the object we're standing on
		velocity_x_ -= stand_info.collide_with->last_move_x()*100 + stand_info.collide_with->platform_motion_x();
		velocity_y_ = 0;

		game_logic::map_formula_callable* callable(new game_logic::map_formula_callable(this));
		callable->add("jumped_on_by", variant(this));
		game_logic::formula_callable_ptr callable_ptr(callable);

		stand_info.collide_with->handle_event(OBJECT_EVENT_JUMPED_ON, callable);
	}

	standing_on_ = stand_info.collide_with;
	if(standing_on_) {
		standing_on_prev_x_ = standing_on_->feet_x();
		standing_on_prev_y_ = standing_on_->feet_y();
	}

	if(lvl.players().empty() == false) {
		lvl.set_touched_player(lvl.players().front());
	}

	if(fall_through_platforms_ > 0) {
		--fall_through_platforms_;
	}

	if(blur_) {
		blur_->next_frame(start_x, start_y, x(), y(), frame_, time_in_frame_, face_right(), upside_down(), start_rotate.as_float(), rotate_.as_float());
		if(blur_->destroyed()) {
			blur_.reset();
		}
	}

	static_process(lvl);
}

void custom_object::static_process(level& lvl)
{
	handle_event(OBJECT_EVENT_PROCESS);
	handle_event(frame_->process_event_id());

	if(type_->timer_frequency() > 0 && (cycle_%type_->timer_frequency()) == 0) {
		static const std::string TimerStr = "timer";
		handle_event(OBJECT_EVENT_TIMER);
	}

	for(std::map<std::string, particle_system_ptr>::iterator i = particle_systems_.begin(); i != particle_systems_.end(); ) {
		i->second->process(*this);
		if(i->second->is_destroyed()) {
			particle_systems_.erase(i++);
		} else {
			++i;
		}
	}

	set_driver_position();

	foreach(const light_ptr& p, lights_) {
		p->process();
	}
}

void custom_object::set_driver_position()
{
	if(driver_) {
		const int pos_right = x() + type_->passenger_x();
		const int pos_left = x() + current_frame().width() - driver_->current_frame().width() - type_->passenger_x();
		driver_->set_face_right(face_right());

		driver_->set_pos(face_right() ? pos_right : pos_left, y() + type_->passenger_y());
	}
}

const_editor_entity_info_ptr custom_object::editor_info() const
{
	return type_->editor_info();
}

int custom_object::zorder() const
{
	return zorder_;
}

int custom_object::zsub_order() const
{
	return zsub_order_;
}

int custom_object::velocity_x() const
{
	return velocity_x_;
}

int custom_object::velocity_y() const
{
	return velocity_y_;
}

int custom_object::surface_friction() const
{
	return type_->surface_friction();
}

int custom_object::surface_traction() const
{
	return type_->surface_traction();
}

bool custom_object::has_feet() const
{
	return has_feet_ && solid();
}

bool custom_object::is_standable(int xpos, int ypos, int* friction, int* traction, int* adjust_y) const
{
	if(!body_passthrough() && !body_harmful() && point_collides(xpos, ypos)) {
		if(friction) {
			*friction = type_->surface_friction();
		}

		if(traction) {
			*traction = type_->surface_traction();
		}

		if(adjust_y) {
			if(type_->use_image_for_collisions()) {
				for(*adjust_y = 0; point_collides(xpos, ypos - *adjust_y - 1); --(*adjust_y)) {
				}
			} else {
				*adjust_y = ypos - body_rect().y();
			}
		}

		return true;
	}

	if(frame_->has_platform()) {
		const frame& f = *frame_;
		int y1 = y() + f.platform_y();
		int y2 = previous_y_ + f.platform_y();

		if(y1 > y2) {
			std::swap(y1, y2);
		}

		if(ypos < y1 || ypos > y2) {
			return false;
		}

		if(xpos < x() + f.platform_x() || xpos >= x() + f.platform_x() + f.platform_w()) {
			return false;
		}

		if(friction) {
			*friction = type_->surface_friction();
		}

		if(traction) {
			*traction = type_->surface_traction();
		}

		if(adjust_y) {
			*adjust_y = y() + f.platform_y() - ypos;
		}

		return true;
	}

	return false;
}

bool custom_object::destroyed() const
{
	return hitpoints_ <= 0;
}

bool custom_object::point_collides(int xpos, int ypos) const
{
	if(type_->use_image_for_collisions()) {
		const bool result = !current_frame().is_alpha(xpos - x(), ypos - y(), time_in_frame_, face_right());
		return result;
	} else {
		return point_in_rect(point(xpos, ypos), body_rect());
	}
}

bool custom_object::rect_collides(const rect& r) const
{
	if(type_->use_image_for_collisions()) {
		rect myrect(x(), y(), current_frame().width(), current_frame().height());
		if(rects_intersect(myrect, r)) {
			rect intersection = intersection_rect(myrect, r);
			for(int y = intersection.y(); y < intersection.y2(); ++y) {
				for(int x = intersection.x(); x < intersection.x2(); ++x) {
					if(point_collides(x, y)) {
						return true;
					}
				}
			}

			return false;
		} else {
			return false;
		}
	} else {
		return rects_intersect(r, body_rect());
	}
}

const_solid_info_ptr custom_object::calculate_solid() const
{
	if(!type_->has_solid()) {
		return const_solid_info_ptr();
	}

	const frame& f = current_frame();
	if(f.solid()) {
		return f.solid();
	}

	return type_->solid();
}

const_solid_info_ptr custom_object::calculate_platform() const
{
	if(platform_solid_info_.get()) {
		return platform_solid_info_;
	} else if(platform_area_) {
		//if platform_solid_info_ is NULL but we have a rect, that
		//means there is no platform, so return NULL instead of
		//defaulting to the type.
		return const_solid_info_ptr();
	}

	return type_->platform();
}

void custom_object::control(const level& lvl)
{
}

custom_object::STANDING_STATUS custom_object::is_standing(const level& lvl, collision_info* info) const
{
	if(!has_feet()) {
		return NOT_STANDING;
	}

	const int width = type_->feet_width();

	if(width >= 1) {
		const int facing = face_right() ? 1 : -1;
		if(point_standable(lvl, *this, feet_x() + width*facing, feet_y(), info, fall_through_platforms_ ? SOLID_ONLY : SOLID_AND_PLATFORMS)) {
			return STANDING_FRONT_FOOT;
		}

		if(point_standable(lvl, *this, feet_x() - width*facing, feet_y(), info, fall_through_platforms_ ? SOLID_ONLY : SOLID_AND_PLATFORMS)) {
			return STANDING_BACK_FOOT;
		}

		return NOT_STANDING;
	}

	if(point_standable(lvl, *this, feet_x(), feet_y(), info, fall_through_platforms_ ? SOLID_ONLY : SOLID_AND_PLATFORMS)) {
		return STANDING_FRONT_FOOT;
	} else {
		return NOT_STANDING;
	}
}

namespace {

#ifndef DISABLE_FORMULA_PROFILER
using formula_profiler::event_call_stack;
#endif

variant call_stack(const custom_object& obj) {
	std::vector<variant> result;

#ifndef DISABLE_FORMULA_PROFILER
	for(int n = 0; n != event_call_stack.size(); ++n) {
		result.push_back(variant(get_object_event_str(event_call_stack[n].event_id)));
	}
#endif

	return variant(&result);
}

}

void custom_object::init()
{
}

variant custom_object::get_value_by_slot(int slot) const
{
	switch(slot) {
	case CUSTOM_OBJECT_CONSTS:            return variant(type_->consts().get());
	case CUSTOM_OBJECT_TYPE:              return variant(type_->id());
	case CUSTOM_OBJECT_ACTIVE:            return variant(last_cycle_active_ >= level::current().cycle() - 2);
	case CUSTOM_OBJECT_TIME_IN_ANIMATION: return variant(time_in_frame_);
	case CUSTOM_OBJECT_TIME_IN_ANIMATION_DELTA: return variant(time_in_frame_delta_);
	case CUSTOM_OBJECT_LEVEL:             return variant(&level::current());
	case CUSTOM_OBJECT_ANIMATION:         return frame_->variant_id();
	case CUSTOM_OBJECT_AVAILABLE_ANIMATIONS: return type_->available_frames();
	case CUSTOM_OBJECT_HITPOINTS:         return variant(hitpoints_);
	case CUSTOM_OBJECT_MAX_HITPOINTS:     return variant(max_hitpoints_);
	case CUSTOM_OBJECT_MASS:              return variant(type_->mass());
	case CUSTOM_OBJECT_LABEL:             return variant(label());
	case CUSTOM_OBJECT_X:                 return variant(x());
	case CUSTOM_OBJECT_Y:                 return variant(y());
	case CUSTOM_OBJECT_XY:                {
			 				 				std::vector<variant> v;
											v.push_back(variant(x()));
											v.push_back(variant(y()));
											return variant(&v);
										  }
	case CUSTOM_OBJECT_Z:
	case CUSTOM_OBJECT_ZORDER:            return variant(zorder_);
	case CUSTOM_OBJECT_ZSUB_ORDER:        return variant(zsub_order_);
	case CUSTOM_OBJECT_RELATIVE_X:        return variant(x() - parent_position().x);
	case CUSTOM_OBJECT_RELATIVE_Y:        return variant(y() - parent_position().y);
	case CUSTOM_OBJECT_PARENT:            return variant(parent_.get());
	case CUSTOM_OBJECT_PIVOT:             return variant(parent_pivot_);
	case CUSTOM_OBJECT_PREVIOUS_Y:        return variant(previous_y_);
	case CUSTOM_OBJECT_X1:                return variant(solid_rect().x());
	case CUSTOM_OBJECT_X2:                return variant(solid_rect().w() ? solid_rect().x2() : x() + current_frame().width());
	case CUSTOM_OBJECT_Y1:                return variant(solid_rect().y());
	case CUSTOM_OBJECT_Y2:                return variant(solid_rect().h() ? solid_rect().y2() : y() + current_frame().height());
	case CUSTOM_OBJECT_W:                 return variant(solid_rect().w());
	case CUSTOM_OBJECT_H:                 return variant(solid_rect().h());

	case CUSTOM_OBJECT_ACTIVATION_BORDER: return variant(activation_border_);
	case CUSTOM_OBJECT_MID_X:
	case CUSTOM_OBJECT_MIDPOINT_X:        return variant(solid_rect().w() ? solid_rect().x() + solid_rect().w()/2 : x() + current_frame().width()/2);
	case CUSTOM_OBJECT_MID_Y:
	case CUSTOM_OBJECT_MIDPOINT_Y:        return variant(solid_rect().h() ? solid_rect().y() + solid_rect().h()/2 : y() + current_frame().height()/2);
	case CUSTOM_OBJECT_SOLID_RECT:        return variant(solid_rect().callable());
	case CUSTOM_OBJECT_SOLID_MID_X:       return variant(solid_rect().x() + solid_rect().w()/2);
	case CUSTOM_OBJECT_SOLID_MID_Y:       return variant(solid_rect().y() + solid_rect().h()/2);
	case CUSTOM_OBJECT_IMG_MID_X:       return variant(x() + current_frame().width()/2);
	case CUSTOM_OBJECT_IMG_MID_Y:       return variant(y() + current_frame().height()/2);
	case CUSTOM_OBJECT_IMG_W:             return variant(current_frame().width());
	case CUSTOM_OBJECT_IMG_H:             return variant(current_frame().height());
	case CUSTOM_OBJECT_FRONT:             return variant(face_right() ? body_rect().x2() : body_rect().x());
	case CUSTOM_OBJECT_BACK:              return variant(face_right() ? body_rect().x() : body_rect().x2());
	case CUSTOM_OBJECT_CYCLE:             return variant(cycle_);
	case CUSTOM_OBJECT_FACING:            return variant(face_right() ? 1 : -1);
	case CUSTOM_OBJECT_UPSIDE_DOWN:       return variant(upside_down() ? 1 : -1);
	case CUSTOM_OBJECT_UP:                return variant(upside_down() ? 1 : -1);
	case CUSTOM_OBJECT_DOWN:              return variant(upside_down() ? -1 : 1);
	case CUSTOM_OBJECT_VELOCITY_X:        return variant(velocity_x_);
	case CUSTOM_OBJECT_VELOCITY_Y:        return variant(velocity_y_);
	case CUSTOM_OBJECT_ACCEL_X:           return variant(accel_x_);
	case CUSTOM_OBJECT_ACCEL_Y:           return variant(accel_y_);
	case CUSTOM_OBJECT_GRAVITY_SHIFT:     return variant(gravity_shift_);
	case CUSTOM_OBJECT_PLATFORM_MOTION_X: return variant(platform_motion_x());
	case CUSTOM_OBJECT_REGISTRY:          return variant(preferences::registry());
	case CUSTOM_OBJECT_GLOBALS:           return variant(global_vars().get());
	case CUSTOM_OBJECT_VARS:              return variant(vars_.get());
	case CUSTOM_OBJECT_TMP:               return variant(tmp_vars_.get());
	case CUSTOM_OBJECT_GROUP:             return variant(group());
	case CUSTOM_OBJECT_ROTATE:            return variant(rotate_);
	case CUSTOM_OBJECT_ME:
	case CUSTOM_OBJECT_SELF:              return variant(this);
	case CUSTOM_OBJECT_BRIGHTNESS:		  return variant((draw_color().r() + draw_color().g() + draw_color().b())/3);
	case CUSTOM_OBJECT_RED:               return variant(draw_color().r());
	case CUSTOM_OBJECT_GREEN:             return variant(draw_color().g());
	case CUSTOM_OBJECT_BLUE:              return variant(draw_color().b());
	case CUSTOM_OBJECT_ALPHA:             return variant(draw_color().a());
	case CUSTOM_OBJECT_TEXT_ALPHA:        return variant(text_ ? text_->alpha : 255);
	case CUSTOM_OBJECT_DAMAGE:            return variant(current_frame().damage());
	case CUSTOM_OBJECT_HIT_BY:            return variant(last_hit_by_.get());
	case CUSTOM_OBJECT_DISTORTION:        return variant(distortion_.get());
	case CUSTOM_OBJECT_IS_STANDING:       return variant(standing_on_.get() || is_standing(level::current()));
	case CUSTOM_OBJECT_STANDING_INFO:     {
		collision_info info;
		is_standing(level::current(), &info);
		if(info.surf_info && info.surf_info->info) {
			return variant(*info.surf_info->info);
		} else {
			return variant();
		}
	}
	case CUSTOM_OBJECT_NEAR_CLIFF_EDGE:   return variant(is_standing(level::current()) && cliff_edge_within(level::current(), feet_x(), feet_y(), face_dir()*15));
	case CUSTOM_OBJECT_DISTANCE_TO_CLIFF: return variant(::distance_to_cliff(level::current(), feet_x(), feet_y(), face_dir()));
	case CUSTOM_OBJECT_SLOPE_STANDING_ON: return variant(-slope_standing_on(6)*face_dir());
	case CUSTOM_OBJECT_UNDERWATER:        return variant(level::current().is_underwater(solid() ? solid_rect() : rect(x(), y(), current_frame().width(), current_frame().height())));
	case CUSTOM_OBJECT_WATER_BOUNDS: {
		rect area;
		if(level::current().is_underwater(solid_rect(), &area)) {
			std::vector<variant> v;
			v.push_back(variant(area.x()));
			v.push_back(variant(area.y()));
			v.push_back(variant(area.x2()));
			v.push_back(variant(area.y2()));
			return variant(&v);
		} else {
			return variant();
		}
	}
	case CUSTOM_OBJECT_WATER_OBJECT: {
		variant v;
		level::current().is_underwater(solid_rect(), NULL, &v);
		return v;
	}
	case CUSTOM_OBJECT_DRIVER:            return variant(driver_ ? driver_.get() : this);
	case CUSTOM_OBJECT_IS_HUMAN:          return variant(is_human() ? 1 : 0);
	case CUSTOM_OBJECT_INVINCIBLE:        return variant(invincible_);
	case CUSTOM_OBJECT_SOUND_VOLUME:      return variant(sound_volume_);
	case CUSTOM_OBJECT_DESTROYED:         return variant(destroyed());

	case CUSTOM_OBJECT_IS_STANDING_ON_PLATFORM: {
		if(standing_on_ && standing_on_->platform() && !standing_on_->solid_platform()) {
			return variant(1);
		}

		collision_info info;
		is_standing(level::current(), &info);
		return variant(info.platform);
	}

	case CUSTOM_OBJECT_STANDING_ON: {
		if(standing_on_) {
			return variant(standing_on_.get());
		}

		entity_ptr stand_on;
		collision_info info;
		is_standing(level::current(), &info);
		return variant(info.collide_with.get());
	}
	
	case CUSTOM_OBJECT_FRAGMENT_SHADERS: {
		std::vector<variant> v;
		foreach(const std::string& shader, fragment_shaders_) {
			v.push_back(variant(shader));
		}

		return variant(&v);
	}

	case CUSTOM_OBJECT_VERTEX_SHADERS: {
		std::vector<variant> v;
		foreach(const std::string& shader, vertex_shaders_) {
			v.push_back(variant(shader));
		}

		return variant(&v);
	}
	
	case CUSTOM_OBJECT_SHADER: {
		if(shader_vars_.get() == NULL) {
			shader_vars_.reset(new game_logic::map_formula_callable);
		}

		return variant(shader_vars_.get());
	}

	case CUSTOM_OBJECT_ACTIVATION_AREA: {
		if(activation_area_.get() != NULL) {
			std::vector<variant> v(4);
			v[0] = variant(activation_area_->x());
			v[1] = variant(activation_area_->y());
			v[2] = variant(activation_area_->w());
			v[3] = variant(activation_area_->h());
			return variant(&v);
		} else {
			return variant();
		}
	}

	case CUSTOM_OBJECT_CLIP_AREA: {
		if(clip_area_.get() != NULL) {
			std::vector<variant> v(4);
			v[0] = variant(clip_area_->x());
			v[1] = variant(clip_area_->y());
			v[2] = variant(clip_area_->w());
			v[3] = variant(clip_area_->h());
			return variant(&v);
		} else {
			return variant();
		}
	}

	case CUSTOM_OBJECT_VARIATIONS: {
		std::vector<variant> result;
		foreach(const std::string& s, current_variation_) {
			result.push_back(variant(s));
		}

		return variant(&result);
	}

	case CUSTOM_OBJECT_ATTACHED_OBJECTS: {
		std::vector<variant> result;
		foreach(const entity_ptr& e, attached_objects()) {
			result.push_back(variant(e.get()));
		}

		return variant(&result);
	}

	case CUSTOM_OBJECT_CALL_STACK: {
		return call_stack(*this);
	}

	case CUSTOM_OBJECT_LIGHTS: {
		std::vector<variant> result;
		foreach(const light_ptr& p, lights_) {
			result.push_back(variant(p.get()));
		}

		return variant(&result);
	}

	case CUSTOM_OBJECT_SOLID_DIMENSIONS_IN: {
		std::vector<variant> v;
		v.push_back(variant(solid_dimensions()));
		v.push_back(variant(weak_solid_dimensions()));
		return variant(&v);
	}

	case CUSTOM_OBJECT_ALWAYS_ACTIVE: return variant(always_active_);
	case CUSTOM_OBJECT_TAGS: return variant(tags_.get());
	case CUSTOM_OBJECT_SCALE:
		if(draw_scale_) {
			return variant(*draw_scale_);
		} else {
			return variant(decimal::from_int(1));
		}
	case CUSTOM_OBJECT_HAS_FEET: return variant(has_feet_);

	case CUSTOM_OBJECT_CTRL_UP:
	case CUSTOM_OBJECT_CTRL_DOWN:
	case CUSTOM_OBJECT_CTRL_LEFT:
	case CUSTOM_OBJECT_CTRL_RIGHT:
	case CUSTOM_OBJECT_CTRL_ATTACK:
	case CUSTOM_OBJECT_CTRL_JUMP:
	case CUSTOM_OBJECT_CTRL_TONGUE:
		return variant(control_status(static_cast<controls::CONTROL_ITEM>(slot - CUSTOM_OBJECT_CTRL_UP)));
	}

	ASSERT_LOG(false, "UNKNOWN SLOT QUERIED FROM OBJECT: " << slot);
}

variant custom_object::get_value(const std::string& key) const
{
	const int slot = type_->callable_definition().get_slot(key);
	if(slot >= 0 && slot < NUM_CUSTOM_OBJECT_PROPERTIES) {
		return get_value_by_slot(slot);
	}

	std::map<std::string, game_logic::const_formula_ptr>::const_iterator property_itor = type_->properties().find(key);
	if(property_itor != type_->properties().end() && property_itor->second) {
		return property_itor->second->execute(*this);
	}

	variant var_result = tmp_vars_->query_value(key);
	if(!var_result.is_null()) {
		return var_result;
	}

	var_result = vars_->query_value(key);
	if(!var_result.is_null()) {
		return var_result;
	}

	std::map<std::string, variant>::const_iterator i = type_->variables().find(key);
	if(i != type_->variables().end()) {
		return i->second;
	}

	std::map<std::string, particle_system_ptr>::const_iterator particle_itor = particle_systems_.find(key);
	if(particle_itor != particle_systems_.end()) {
		return variant(particle_itor->second.get());
	}

	if(backup_callable_stack_.empty() == false && backup_callable_stack_.top()) {
		if(key == "arg") {
			return variant(backup_callable_stack_.top());
		}

		return backup_callable_stack_.top()->query_value(key);
	}

	return variant();
}

void custom_object::get_inputs(std::vector<game_logic::formula_input>* inputs) const
{
	inputs->push_back(game_logic::formula_input("time_in_animation", game_logic::FORMULA_READ_WRITE));
	inputs->push_back(game_logic::formula_input("level", game_logic::FORMULA_READ_ONLY));
	inputs->push_back(game_logic::formula_input("animation", game_logic::FORMULA_READ_ONLY));
	inputs->push_back(game_logic::formula_input("hitpoints", game_logic::FORMULA_READ_WRITE));
}

void custom_object::set_value(const std::string& key, const variant& value)
{
	const int slot = custom_object_callable::get_key_slot(key);
	if(slot != -1) {
		set_value_by_slot(slot, value);
		return;
	}

	if(key == "animation") {
		set_frame(value.as_string());
	} else if(key == "time_in_animation") {
		time_in_frame_ = value.as_int()%frame_->duration();
	} else if(key == "time_in_animation_delta") {
		time_in_frame_delta_ = value.as_int();
	} else if(key == "x") {
		const int start_x = centi_x();
		set_x(value.as_int());
		if(entity_collides(level::current(), *this, MOVE_NONE) && entity_in_current_level(this)) {
			set_centi_x(start_x);
		}
	} else if(key == "y") {
		const int start_y = centi_y();
		set_y(value.as_int());
		if(entity_collides(level::current(), *this, MOVE_NONE) && entity_in_current_level(this)) {
			set_centi_y(start_y);
		}
	} else if(key == "xy") {
		const int start_x = centi_x();
		const int start_y = centi_y();
		set_x(value[0].as_int());
		set_y(value[1].as_int());
		if(entity_collides(level::current(), *this, MOVE_NONE) && entity_in_current_level(this)) {
			set_centi_x(start_x);
			set_centi_y(start_y);
		}
	} else if(key == "z" || key == "zorder") {
		zorder_ = value.as_int();
	} else if(key == "zsub_order") {
		zsub_order_ = value.as_int();
	} else if(key == "midpoint_x") {
		const int current_x = x() + current_frame().width()/2;
		const int xdiff = current_x - x();
		set_pos(value.as_int() - xdiff, y());
	} else if(key == "midpoint_y") {
		const int current_y = y() + current_frame().height()/2;
		const int ydiff = current_y - y();
		set_pos(x(), value.as_int() - ydiff);
	} else if(key == "facing") {
		set_face_right(value.as_int() > 0);
	} else if(key == "upside_down") {
		set_upside_down(value.as_int());
	} else if(key == "hitpoints") {
		const int old_hitpoints = hitpoints_;
		hitpoints_ = value.as_int();
		if(old_hitpoints > 0 && hitpoints_ <= 0) {
			die();
		}
	} else if(key == "max_hitpoints") {
		max_hitpoints_ = value.as_int();
		if(hitpoints_ > max_hitpoints_) {
			hitpoints_ = max_hitpoints_;
		}
	} else if(key == "velocity_x") {
		velocity_x_ = value.as_int();
	} else if(key == "velocity_y") {
		velocity_y_ = value.as_int();
	} else if(key == "accel_x") {
		accel_x_ = value.as_int();
	} else if(key == "accel_y") {
		accel_y_ = value.as_int();
	} else if(key == "rotate") {
		rotate_ = value.as_decimal();
	} else if(key == "red") {
		make_draw_color();
		draw_color_->buf()[0] = truncate_to_char(value.as_int());
	} else if(key == "green") {
		make_draw_color();
		draw_color_->buf()[1] = truncate_to_char(value.as_int());
	} else if(key == "blue") {
		make_draw_color();
		draw_color_->buf()[2] = truncate_to_char(value.as_int());
	} else if(key == "alpha") {
		make_draw_color();
		draw_color_->buf()[3] = truncate_to_char(value.as_int());
	} else if(key == "brightness"){
		make_draw_color();
		draw_color_->buf()[0] = value.as_int();
		draw_color_->buf()[1] = value.as_int();
		draw_color_->buf()[2] = value.as_int();
	} else if(key == "distortion") {
		distortion_ = value.try_convert<graphics::raster_distortion>();
	} else if(key == "current_generator") {
		set_current_generator(value.try_convert<current_generator>());
	} else if(key == "invincible") {
		invincible_ = value.as_int();
	} else if(key == "fall_through_platforms") {
		fall_through_platforms_ = value.as_int();
	} else if(key == "tags") {
		if(value.is_list()) {
			tags_ = new game_logic::map_formula_callable;
			for(int n = 0; n != value.num_elements(); ++n) {
				tags_->add(value[n].as_string(), variant(1));
			}
		}
	} else if(key == "fragment_shaders") {
		fragment_shaders_.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			fragment_shaders_.push_back(value[n].as_string());
		}
		shader_ = 0;
	} else if(key == "vertex_shaders") {
		vertex_shaders_.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			vertex_shaders_.push_back(value[n].as_string());
		}
		shader_ = 0;
	} else if(key == "draw_area") {
		if(value.is_list() && value.num_elements() == 4) {
			draw_area_.reset(new rect(value[0].as_int(), value[1].as_int(), value[2].as_int(), value[3].as_int()));
		} else {
			draw_area_.reset();
		}
	} else if(key == "scale") {
		draw_scale_.reset(new decimal(value.as_decimal()));
		if(draw_scale_->as_int() == 1 && draw_scale_->fractional() == 0) {
			draw_scale_.reset();
		}
	} else if(key == "activation_area") {
		if(value.is_list() && value.num_elements() == 4) {
			activation_area_.reset(new rect(value[0].as_int(), value[1].as_int(), value[2].as_int(), value[3].as_int()));
		} else {
			ASSERT_LOG(value.is_null(), "BAD ACTIVATION AREA: " << value.to_debug_string());
			activation_area_.reset();
		}
	} else if(key == "clip_area") {
		if(value.is_list() && value.num_elements() == 4) {
			clip_area_.reset(new rect(value[0].as_int(), value[1].as_int(), value[2].as_int(), value[3].as_int()));
		} else {
			ASSERT_LOG(value.is_null(), "BAD CLIP AREA: " << value.to_debug_string());
			clip_area_.reset();
		}
	} else if(key == "variations") {
		handle_event("reset_variations");
		current_variation_.clear();
		if(value.is_list()) {
			for(int n = 0; n != value.num_elements(); ++n) {
				current_variation_.push_back(value[n].as_string());
			}
		} else if(value.is_string()) {
			current_variation_.push_back(value.as_string());
		}

		if(current_variation_.empty()) {
			type_ = base_type_;
		} else {
			type_ = base_type_->get_variation(current_variation_);
		}

		calculate_solid_rect();

		handle_event("set_variations");
	} else if(key == "attached_objects") {
		std::vector<entity_ptr> v;
		for(int n = 0; n != value.num_elements(); ++n) {
			entity* e = value[n].try_convert<entity>();
			if(e) {
				v.push_back(entity_ptr(e));
			}
		}

		set_attached_objects(v);
	} else if(key == "solid_dimensions_in" || key == "solid_dimensions_not_in") {

		unsigned int solid = 0, weak = 0;
		for(int n = 0; n != value.num_elements(); ++n) {
			std::string str = value[n].as_string();
			if(!str.empty() && str[0] == '~') {
				str = std::string(str.begin() + 1, str.end());
				const int id = get_solid_dimension_id(str);
				weak |= 1 << id;
			} else {
				const int id = get_solid_dimension_id(value[n].as_string());
				solid |= 1 << id;
			}
		}

		if(key == "solid_dimensions_not_in") {
			solid = ~solid;
			weak = ~weak;
		}

		weak |= solid;

		const unsigned int old_solid = solid_dimensions();
		const unsigned int old_weak = weak_solid_dimensions();
		set_solid_dimensions(solid, weak);
		collision_info collide_info;
		if(entity_in_current_level(this) && entity_collides(level::current(), *this, MOVE_NONE, &collide_info)) {
			set_solid_dimensions(old_solid, old_weak);
			ASSERT_EQ(entity_collides(level::current(), *this, MOVE_NONE), false);

			game_logic::map_formula_callable* callable(new game_logic::map_formula_callable(this));
			callable->add("collide_with", variant(collide_info.collide_with.get()));
			game_logic::formula_callable_ptr callable_ptr(callable);

			handle_event(OBJECT_EVENT_CHANGE_SOLID_DIMENSIONS_FAIL, callable);
		}

	} else if(key == "xscale" || key == "yscale") {
		if(parallax_scale_millis_.get() == NULL) {
			parallax_scale_millis_.reset(new std::pair<int,int>(1000,1000));
		}

		const int v = value.as_int();

		if(key == "xscale") {
			const int current = (parallax_scale_millis_->first*x())/1000;
			const int new_value = (v*current)/1000;
			set_x(new_value);
			parallax_scale_millis_->first = v;
		} else {
			const int current = (parallax_scale_millis_->second*y())/1000;
			const int new_value = (v*current)/1000;
			set_y(new_value);
			parallax_scale_millis_->second = v;
		}
	} else if(key == "type") {
		const_custom_object_type_ptr p = custom_object_type::get(value.as_string());
		if(p) {
			game_logic::formula_variable_storage_ptr old_vars = vars_, old_tmp_vars_ = tmp_vars_;

			type_ = p;
			has_feet_ = type_->has_feet();
			vars_.reset(new game_logic::formula_variable_storage(type_->variables())),
			tmp_vars_.reset(new game_logic::formula_variable_storage(type_->tmp_variables())),

			vars_->add(*old_vars);
			tmp_vars_->add(*old_tmp_vars_);

			//set the animation to the default animation for the new type.
			set_frame(type_->default_frame().id());
			//std::cerr << "SET TYPE WHEN CHANGING TO '" << type_->id() << "'\n";
		}
	} else {
		vars_->add(key, value);
	}
}

void custom_object::set_value_by_slot(int slot, const variant& value)
{
	switch(slot) {
	case CUSTOM_OBJECT_TYPE: {
		const_custom_object_type_ptr p = custom_object_type::get(value.as_string());
		if(p) {
			game_logic::formula_variable_storage_ptr old_vars = vars_, old_tmp_vars_ = tmp_vars_;

			type_ = p;
			has_feet_ = type_->has_feet();
			vars_.reset(new game_logic::formula_variable_storage(type_->variables())),
			tmp_vars_.reset(new game_logic::formula_variable_storage(type_->tmp_variables())),

			vars_->add(*old_vars);
			tmp_vars_->add(*old_tmp_vars_);

			//set the animation to the default animation for the new type.
			set_frame(type_->default_frame().id());
			//std::cerr << "SET TYPE WHEN CHANGING TO '" << type_->id() << "'\n";
		}
	}
		break;
	case CUSTOM_OBJECT_TIME_IN_ANIMATION:
		time_in_frame_ = value.as_int()%frame_->duration();
		break;
	case CUSTOM_OBJECT_TIME_IN_ANIMATION_DELTA:
		time_in_frame_delta_ = value.as_int();
		break;
	case CUSTOM_OBJECT_ANIMATION:
		set_frame(value.as_string());
		break;
	
	case CUSTOM_OBJECT_X: {
		const int start_x = centi_x();
		set_x(value.as_int());
		if(entity_collides(level::current(), *this, MOVE_NONE) && entity_in_current_level(this)) {
			set_centi_x(start_x);
		}

		break;
	}
	
	case CUSTOM_OBJECT_Y: {
		const int start_y = centi_y();
		set_y(value.as_int());
		if(entity_collides(level::current(), *this, MOVE_NONE) && entity_in_current_level(this)) {
			set_centi_y(start_y);
		}

		break;
	}
	
	case CUSTOM_OBJECT_XY: {
		ASSERT_LOG(value.is_list() && value.num_elements() == 2, "set xy value of object to a value in incorrect format ([x,y] expected): " << value.to_debug_string());
		const int start_x = centi_x();
		const int start_y = centi_y();
		set_x(value[0].as_int());
		set_y(value[1].as_int());
		if(entity_collides(level::current(), *this, MOVE_NONE) && entity_in_current_level(this)) {
			set_centi_x(start_x);
			set_centi_y(start_y);
		}

		break;
	}

	case CUSTOM_OBJECT_Z:
	case CUSTOM_OBJECT_ZORDER:
		zorder_ = value.as_int();
		break;
		
	case CUSTOM_OBJECT_ZSUB_ORDER:
		zsub_order_ = value.as_int();
		break;
	
	case CUSTOM_OBJECT_RELATIVE_X: {
		const point p = parent_position();
		set_value_by_slot(CUSTOM_OBJECT_X, variant(p.x + value.as_int()));
		break;
	}

	case CUSTOM_OBJECT_RELATIVE_Y: {
		const point p = parent_position();
		set_value_by_slot(CUSTOM_OBJECT_Y, variant(p.y + value.as_int()));
		break;
	}

	case CUSTOM_OBJECT_PARENT: {
		entity_ptr e(value.try_convert<entity>());
		set_parent(e, parent_pivot_);
		break;
	}

	case CUSTOM_OBJECT_PIVOT: {
		set_parent(parent_, value.as_string());
		break;
	}
	
	case CUSTOM_OBJECT_MIDPOINT_X: {
		const int start_x = centi_x();
		const int current_x = x() + current_frame().width()/2;
		const int xdiff = current_x - x();
		set_pos(value.as_int() - xdiff, y());
		if(entity_collides(level::current(), *this, MOVE_NONE) && entity_in_current_level(this)) {
			set_centi_x(start_x);
		}
		break;
	}

	case CUSTOM_OBJECT_MIDPOINT_Y: {
		const int start_y = centi_y();
		const int current_y = y() + current_frame().height()/2;
		const int ydiff = current_y - y();
		set_pos(x(), value.as_int() - ydiff);
		if(entity_collides(level::current(), *this, MOVE_NONE) && entity_in_current_level(this)) {
			set_centi_y(start_y);
		}
		break;
	}

	case CUSTOM_OBJECT_CYCLE:
		cycle_ = value.as_int();
		break;

	case CUSTOM_OBJECT_FACING:
		set_face_right(value.as_int() > 0);
		break;
	
	case CUSTOM_OBJECT_UPSIDE_DOWN:
		set_upside_down(value.as_int());
		break;

	case CUSTOM_OBJECT_HITPOINTS: {
		const int old_hitpoints = hitpoints_;
		hitpoints_ = value.as_int();
		if(old_hitpoints > 0 && hitpoints_ <= 0) {
			die();
		}
		break;
	}
	case CUSTOM_OBJECT_MAX_HITPOINTS:
		max_hitpoints_ = value.as_int();
		if(hitpoints_ > max_hitpoints_) {
			hitpoints_ = max_hitpoints_;
		}
		break;

	case CUSTOM_OBJECT_VELOCITY_X:
		velocity_x_ = value.as_int();
		break;
	
	case CUSTOM_OBJECT_VELOCITY_Y:
		velocity_y_ = value.as_int();
		break;

	case CUSTOM_OBJECT_ACCEL_X:
		accel_x_ = value.as_int();
		break;

	case CUSTOM_OBJECT_ACCEL_Y:
		accel_y_ = value.as_int();
		break;

	case CUSTOM_OBJECT_GRAVITY_SHIFT:
		gravity_shift_ = value.as_int();
		break;

	case CUSTOM_OBJECT_PLATFORM_MOTION_X:
		set_platform_motion_x(value.as_int());
		break;

	case CUSTOM_OBJECT_ROTATE:
		rotate_ = value.as_decimal();
		break;

	case CUSTOM_OBJECT_RED:
		make_draw_color();
		draw_color_->buf()[0] = truncate_to_char(value.as_int());
		break;
	
	case CUSTOM_OBJECT_GREEN:
		make_draw_color();
		draw_color_->buf()[1] = truncate_to_char(value.as_int());
		break;
	
	case CUSTOM_OBJECT_BLUE:
		make_draw_color();
		draw_color_->buf()[2] = truncate_to_char(value.as_int());
		break;

	case CUSTOM_OBJECT_ALPHA:
		make_draw_color();
		draw_color_->buf()[3] = truncate_to_char(value.as_int());
		break;

	case CUSTOM_OBJECT_TEXT_ALPHA:
		if(!text_) {
			set_text("", "default", 10, false);
		}

		text_->alpha = value.as_int();
		break;

	case CUSTOM_OBJECT_BRIGHTNESS:
		make_draw_color();
		draw_color_->buf()[0] = value.as_int();
		draw_color_->buf()[1] = value.as_int();
		draw_color_->buf()[2] = value.as_int();
		break;
	
	case CUSTOM_OBJECT_DISTORTION:
		distortion_ = value.try_convert<graphics::raster_distortion>();
		break;
	
	case CUSTOM_OBJECT_CURRENT_GENERATOR:
		set_current_generator(value.try_convert<current_generator>());
		break;

	case CUSTOM_OBJECT_INVINCIBLE:
		invincible_ = value.as_int();
		break;
	
	case CUSTOM_OBJECT_FALL_THROUGH_PLATFORMS:
		fall_through_platforms_ = value.as_int();
		break;
	
	case CUSTOM_OBJECT_HAS_FEET:
		has_feet_ = value.as_bool();
		break;
	
	case CUSTOM_OBJECT_TAGS:
		if(value.is_list()) {
			tags_ = new game_logic::map_formula_callable;
			for(int n = 0; n != value.num_elements(); ++n) {
				tags_->add(value[n].as_string(), variant(1));
			}
		}

		break;
	
	case CUSTOM_OBJECT_FRAGMENT_SHADERS:
		fragment_shaders_.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			fragment_shaders_.push_back(value[n].as_string());
		}
		shader_ = 0;
		break;

	case CUSTOM_OBJECT_VERTEX_SHADERS:
		vertex_shaders_.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			vertex_shaders_.push_back(value[n].as_string());
		}
		shader_ = 0;
		break;

	case CUSTOM_OBJECT_DRAW_AREA:
		if(value.is_list() && value.num_elements() == 4) {
			draw_area_.reset(new rect(value[0].as_int(), value[1].as_int(), value[2].as_int(), value[3].as_int()));
		} else {
			draw_area_.reset();
		}

		break;

	case CUSTOM_OBJECT_SCALE:
		draw_scale_.reset(new decimal(value.as_decimal()));
		if(draw_scale_->as_int() == 1 && draw_scale_->fractional() == 0) {
			draw_scale_.reset();
		}

		break;
	
	case CUSTOM_OBJECT_ACTIVATION_BORDER:
		activation_border_ = value.as_int();
	
		break;

			
	case CUSTOM_OBJECT_ACTIVATION_AREA:
		if(value.is_list() && value.num_elements() == 4) {
			activation_area_.reset(new rect(value[0].as_int(), value[1].as_int(), value[2].as_int(), value[3].as_int()));
		} else {
			ASSERT_LOG(value.is_null(), "BAD ACTIVATION AREA: " << value.to_debug_string());
			activation_area_.reset();
		}

		break;
	
	case CUSTOM_OBJECT_CLIP_AREA:
		if(value.is_list() && value.num_elements() == 4) {
			clip_area_.reset(new rect(value[0].as_int(), value[1].as_int(), value[2].as_int(), value[3].as_int()));
		} else {
			ASSERT_LOG(value.is_null(), "BAD CLIP AREA: " << value.to_debug_string());
			clip_area_.reset();
		}

		break;

	case CUSTOM_OBJECT_ALWAYS_ACTIVE:
		always_active_ = value.as_bool();
		break;
			
	case CUSTOM_OBJECT_VARIATIONS:
		handle_event("reset_variations");
		current_variation_.clear();
		if(value.is_list()) {
			for(int n = 0; n != value.num_elements(); ++n) {
				current_variation_.push_back(value[n].as_string());
			}
		} else if(value.is_string()) {
			current_variation_.push_back(value.as_string());
		}

		if(current_variation_.empty()) {
			type_ = base_type_;
		} else {
			type_ = base_type_->get_variation(current_variation_);
		}

		calculate_solid_rect();
		handle_event("set_variations");
		break;
	
	case CUSTOM_OBJECT_ATTACHED_OBJECTS: {
		std::vector<entity_ptr> v;
		for(int n = 0; n != value.num_elements(); ++n) {
			entity* e = value[n].try_convert<entity>();
			if(e) {
				v.push_back(entity_ptr(e));
			}
		}

		set_attached_objects(v);
		break;
	}

	case CUSTOM_OBJECT_COLLIDE_DIMENSIONS_IN:
	case CUSTOM_OBJECT_COLLIDE_DIMENSIONS_NOT_IN: {
		unsigned int solid = 0, weak = 0;
		for(int n = 0; n != value.num_elements(); ++n) {
			std::string str = value[n].as_string();
			if(!str.empty() && str[0] == '~') {
				str = std::string(str.begin() + 1, str.end());
				const int id = get_solid_dimension_id(str);
				weak |= 1 << id;
			} else {
				const int id = get_solid_dimension_id(value[n].as_string());
				solid |= 1 << id;
			}
		}

		if(slot == CUSTOM_OBJECT_COLLIDE_DIMENSIONS_NOT_IN) {
			solid = ~solid;
			weak = ~weak;
		}

		weak |= solid;

		set_collide_dimensions(solid, weak);
		break;
	}

	case CUSTOM_OBJECT_LIGHTS: {
		lights_.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			light* p = value[n].try_convert<light>();
			if(p) {
				lights_.push_back(light_ptr(p));
			}
		}
		break;
	}

	case CUSTOM_OBJECT_SOLID_DIMENSIONS_IN:
	case CUSTOM_OBJECT_SOLID_DIMENSIONS_NOT_IN: {
		unsigned int solid = 0, weak = 0;
		for(int n = 0; n != value.num_elements(); ++n) {
			std::string str = value[n].as_string();
			if(!str.empty() && str[0] == '~') {
				str = std::string(str.begin() + 1, str.end());
				const int id = get_solid_dimension_id(str);
				weak |= 1 << id;
			} else {
				const int id = get_solid_dimension_id(value[n].as_string());
				solid |= 1 << id;
			}
		}

		if(slot == CUSTOM_OBJECT_SOLID_DIMENSIONS_NOT_IN) {
			solid = ~solid;
			weak = ~weak;
		}

		weak |= solid;

		const unsigned int old_solid = solid_dimensions();
		const unsigned int old_weak = weak_solid_dimensions();
		set_solid_dimensions(solid, weak);
		collision_info collide_info;
		if(entity_in_current_level(this) && entity_collides(level::current(), *this, MOVE_NONE, &collide_info)) {
			set_solid_dimensions(old_solid, old_weak);
			ASSERT_EQ(entity_collides(level::current(), *this, MOVE_NONE), false);

			game_logic::map_formula_callable* callable(new game_logic::map_formula_callable(this));
			callable->add("collide_with", variant(collide_info.collide_with.get()));
			game_logic::formula_callable_ptr callable_ptr(callable);

			handle_event(OBJECT_EVENT_CHANGE_SOLID_DIMENSIONS_FAIL, callable);
		}

		break;
	}

	case CUSTOM_OBJECT_X_SCHEDULE: {
		if(position_schedule_.get() == NULL) {
			position_schedule_.reset(new position_schedule);
		}

		position_schedule_->x_pos.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			position_schedule_->x_pos.push_back(value[n].as_int());
		}
		break;
	}
	case CUSTOM_OBJECT_Y_SCHEDULE: {
		if(position_schedule_.get() == NULL) {
			position_schedule_.reset(new position_schedule);
		}

		position_schedule_->y_pos.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			position_schedule_->y_pos.push_back(value[n].as_int());
		}
		break;
	}
	case CUSTOM_OBJECT_ROTATION_SCHEDULE: {
		if(position_schedule_.get() == NULL) {
			position_schedule_.reset(new position_schedule);
		}

		position_schedule_->rotation.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			position_schedule_->rotation.push_back(value[n].as_decimal());
		}
		break;
	}

	case CUSTOM_OBJECT_SCHEDULE_SPEED: {
		if(position_schedule_.get() == NULL) {
			position_schedule_.reset(new position_schedule);
		}

		position_schedule_->speed = value.as_int();

		break;
	}

	case CUSTOM_OBJECT_PLATFORM_AREA: {
		ASSERT_GE(value.num_elements(), 3);
		ASSERT_LE(value.num_elements(), 4);

		set_platform_area(rect(value[0].as_int(), value[1].as_int(), value[2].as_int(), 1));
		calculate_solid_rect();
		break;
	}

	case CUSTOM_OBJECT_PLATFORM_OFFSETS: {
		platform_offsets_.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			platform_offsets_.push_back(value[n].as_int());
		}
		break;
	}

	case CUSTOM_OBJECT_CUSTOM_DRAW: {
		if(value.is_null()) {
			custom_draw_.reset();
		}

		std::vector<frame::CustomPoint>* v = new std::vector<frame::CustomPoint>;

		custom_draw_.reset(v);

		std::vector<GLfloat> positions;

		for(int n = 0; n != value.num_elements(); ++n) {
			if(value[n].is_decimal() || value[n].is_int()) {
				positions.push_back(value[n].as_decimal().as_float());
			} else if(value[n].is_list()) {
				for(int index = 0; index != value[n].num_elements(); index += 2) {
					ASSERT_LOG(value[n].num_elements() - index >= 2, "ILLEGAL VALUE TO custom_draw: " << value.to_debug_string() << ", " << n << ", " << index << "/" << value[n].num_elements());

					ASSERT_LOG(v->size() < positions.size(), "ILLEGAL VALUE TO custom_draw -- not enough positions for number of offsets: " << value.to_debug_string() << " " << v->size() << " VS " << positions.size());
					const GLfloat pos = positions[v->size()];

					v->push_back(frame::CustomPoint());
					v->back().pos = pos;
					v->back().offset = point(value[n][index].as_int(), value[n][index + 1].as_int());
				}
			}
		}

		ASSERT_LOG(v->size() >= 3, "ILLEGAL VALUE TO custom_draw: " << value.to_debug_string());

		std::vector<frame::CustomPoint> draw_order;
		int n1 = 0, n2 = v->size() - 1;
		while(n1 <= n2) {
			draw_order.push_back((*v)[n1]);
			if(n2 > n1) {
				draw_order.push_back((*v)[n2]);
			}

			++n1;
			--n2;
		}

		v->swap(draw_order);

		break;
	}

	default:
		break;

	}
}

void custom_object::set_frame(const std::string& name)
{
	const std::string previous_animation = frame_name_;

	const bool changing_anim = name != frame_name_;

	//fire an event to say that we're leaving the current frame.
	if(frame_ && changing_anim) {
		handle_event(frame_->leave_event_id());
	}

	const int start_x = feet_x();
	const int start_y = feet_y();

	frame_ = &type_->get_frame(name);
	calculate_solid_rect();
	++current_animation_id_;

	const int diff_x = feet_x() - start_x;
	const int diff_y = feet_y() - start_y;

	if(type_->adjust_feet_on_animation_change()) {
		move_centipixels(-diff_x*100, -diff_y*100);
	}

	set_frame_no_adjustments(name);

	frame_->play_sound(this);

	if(entity_collides(level::current(), *this, MOVE_NONE) && entity_in_current_level(this)) {
		game_logic::map_formula_callable* callable(new game_logic::map_formula_callable);
		callable->add("previous_animation", variant(previous_animation));
		game_logic::formula_callable_ptr callable_ptr(callable);
		static int change_animation_failure_recurse = 0;
		ASSERT_LOG(change_animation_failure_recurse < 5, "OBJECT " << type_->id() << " FAILS TO RESOLVE ANIMATION CHANGE FAILURES");
		++change_animation_failure_recurse;
		handle_event(OBJECT_EVENT_CHANGE_ANIMATION_FAILURE, callable);
		handle_event("change_animation_failure_" + frame_name_, callable);
		--change_animation_failure_recurse;
		ASSERT_LOG(destroyed() || !entity_collides(level::current(), *this, MOVE_NONE),
		  "Object '" << type_->id() << "' has different solid areas when changing from frame " << previous_animation << " to " << frame_name_ << " and doesn't handle it properly");
	}

	handle_event(OBJECT_EVENT_ENTER_ANIM);
	handle_event(frame_->enter_event_id());
}

rect custom_object::draw_rect() const
{
	if(draw_area_) {
		return rect(x(), y(), draw_area_->w()*2, draw_area_->h()*2);
	} else {
		return rect(x(), y(), frame_->width(), frame_->height());
	}
}

void custom_object::set_frame_no_adjustments(const std::string& name)
{
	frame_ = &type_->get_frame(name);
	frame_name_ = name;
	time_in_frame_ = 0;
	if(frame_->velocity_x() != INT_MIN) {
		velocity_x_ = frame_->velocity_x() * (face_right() ? 1 : -1);
	}

	if(frame_->velocity_y() != INT_MIN) {
		velocity_y_ = frame_->velocity_y();
	}

	if(frame_->accel_x() != INT_MIN) {
		accel_x_ = frame_->accel_x();
	}
	
	if(frame_->accel_y() != INT_MIN) {
		accel_y_ = frame_->accel_y();
	}

	calculate_solid_rect();
}

void custom_object::die()
{
	hitpoints_ = 0;
	handle_event(OBJECT_EVENT_DIE);
}

void custom_object::die_with_no_event()
{
	hitpoints_ = 0;
}


bool custom_object::is_active(const rect& screen_area) const
{
	if(controls::num_players() > 1) {
		//in multiplayer, make all objects always active
		//TODO: review this behavior
		return true;
	}

	if(always_active()) {
		return true;
	}

	if(type_->goes_inactive_only_when_standing() && !is_standing(level::current())) {
		return true;
	}

	if(activation_area_) {
		return rects_intersect(*activation_area_, screen_area);
	}

	if(text_) {
		const rect text_area(x(), y(), text_->dimensions.w(), text_->dimensions.h());
		if(rects_intersect(screen_area, text_area)) {
			return true;
		}
	}

	const rect& area = frame_rect();
	if(draw_area_) {
		rect draw_area(area.x(), area.y(), draw_area_->w()*2, draw_area_->h()*2);
		return rects_intersect(draw_area, screen_area);
	}
	
	if(parallax_scale_millis_.get() != NULL) {
		if(parallax_scale_millis_->first != 1000 || parallax_scale_millis_->second != 1000){
			const int diffx = ((parallax_scale_millis_->first - 1000)*screen_area.x())/1000;
			const int diffy = ((parallax_scale_millis_->second - 1000)*screen_area.y())/1000;
			rect screen(screen_area.x() - diffx, screen_area.y() - diffy,
						screen_area.w(), screen_area.h());
			const rect& area = frame_rect();
			return rects_intersect(screen, area);
		}
	}

	const int border = activation_border_;
	if(area.x() < screen_area.x2() + border &&
	   area.x2() > screen_area.x() - border &&
	   area.y() < screen_area.y2() + border &&
	   area.y2() > screen_area.y() - border) {
		return true;
	}

	
	return false;
}

bool custom_object::move_to_standing(level& lvl, int max_displace)
{
	int start_y = y();
	const bool result = move_to_standing_internal(lvl, max_displace);
	if(!result || entity_collides(level::current(), *this, MOVE_NONE)) {
		set_pos(x(), start_y);
		return false;
	}

	return result;
}

bool custom_object::move_to_standing_internal(level& lvl, int max_displace)
{
	int start_y = y();
	//descend from the initial-position (what the player was at in the prev level) until we're standing
	for(int n = 0; n != max_displace; ++n) {
		if(is_standing(lvl)) {
			
			if(n == 0) {  //if we've somehow managed to be standing on the very first frame, try to avoid the possibility that this is actually some open space underground on a cave level by scanning up till we reach the surface.
				for(int n = 0; n != max_displace; ++n) {
					set_pos(x(), y() - 1);
					if(!is_standing(lvl)) {
						set_pos(x(), y() + 1);
						
						if(y() < lvl.boundaries().y()) {
							//we are too high, out of the level. Move the
							//character down, under the solid, and then
							//call this function again to move them down
							//to standing on the solid below.
							for(int n = 0; n != max_displace; ++n) {
								set_pos(x(), y() + 1);
								if(!is_standing(lvl)) {
									return move_to_standing_internal(lvl, max_displace);
								}
							}
						}
						
						return true;
					}
				}
				return true;
			}
			return true;
		}
		
		set_pos(x(), y() + 1);
	}
	
	set_pos(x(), start_y);
	return false;
}


bool custom_object::dies_on_inactive() const
{
	return type_->dies_on_inactive();
}

bool custom_object::always_active() const
{
	return always_active_ || type_->always_active();
}

bool custom_object::body_harmful() const
{
	return type_->body_harmful();
}

bool custom_object::body_passthrough() const
{
	return type_->body_passthrough();
}

const frame& custom_object::icon_frame() const
{
	return type_->default_frame();
}

entity_ptr custom_object::clone() const
{
	entity_ptr res(new custom_object(*this));
	res->set_distinct_label();
	return res;
}

entity_ptr custom_object::backup() const
{
	entity_ptr res(new custom_object(*this));
	return res;
}

void custom_object::handle_event(const std::string& event, const formula_callable* context)
{
	handle_event(get_object_event_id(event), context);
}

void custom_object::handle_event(int event, const formula_callable* context)
{
	if(hitpoints_ <= 0 && event != OBJECT_EVENT_DIE) {
		return;
	}

	const game_logic::formula* handlers[2];
	int nhandlers = 0;

	if(event < event_handlers_.size() && event_handlers_[event]) {
		handlers[nhandlers++] = event_handlers_[event].get();
	}

	const game_logic::formula* type_handler = type_->get_event_handler(event).get();
	if(type_handler != NULL) {
		handlers[nhandlers++] = type_handler;
	}

	if(!nhandlers) {
		return;
	}

	backup_callable_stack_.push(context);

	for(int n = 0; n != nhandlers; ++n) {
		const game_logic::formula* handler = handlers[n];

#ifndef DISABLE_FORMULA_PROFILER
		formula_profiler::custom_object_event_frame event_frame = { type_.get(), event, false };
		event_call_stack.push_back(event_frame);
#endif

		++events_handled_per_second;

		variant var = handler->execute(*this);

#ifndef DISABLE_FORMULA_PROFILER
		event_call_stack.back().executing_commands = true;
#endif

		const bool result = execute_command(var);
#ifndef DISABLE_FORMULA_PROFILER
		event_call_stack.pop_back();
#endif
		if(!result) {
			break;
		}
	}

	backup_callable_stack_.pop();
}

bool custom_object::execute_command(const variant& var)
{
	bool result = true;
	if(var.is_null()) { return result; }
	if(var.is_list()) {
		const int num_elements = var.num_elements();
		for(int n = 0; n != num_elements; ++n) {
			result = execute_command(var[n]) && result;
		}
	} else {
		custom_object_command_callable* cmd = var.try_convert<custom_object_command_callable>();
		if(cmd != NULL) {
			cmd->execute(level::current(), *this);
		} else {
			entity_command_callable* cmd = var.try_convert<entity_command_callable>();
			if(cmd != NULL) {
				cmd->execute(level::current(), *this);
			} else {
				if(var.try_convert<swallow_object_command_callable>()) {
					result = false;
				}
			}
		}
	}

	return result;
}

int custom_object::slope_standing_on(int range) const
{
	if(!is_standing(level::current())) {
		return 0;
	}

	const int forward = face_right() ? 1 : -1;
	const int xpos = feet_x();
	int ypos = feet_y();


	for(int n = 0; !level::current().standable(xpos, ypos) && n != 10; ++n) {
		++ypos;
	}

	if(range == 1) {
		if(level::current().standable(xpos + forward, ypos - 1) &&
		   !level::current().standable(xpos - forward, ypos)) {
			return 45;
		}

		if(!level::current().standable(xpos + forward, ypos) &&
		   level::current().standable(xpos - forward, ypos - 1)) {
			return -45;
		}

		return 0;
	} else {
		if(!is_standing(level::current())) {
			return 0;
		}

		int y1 = find_ground_level(level::current(), xpos + forward*range, ypos, range+1);
		int y2 = find_ground_level(level::current(), xpos - forward*range, ypos, range+1);
		while((y1 == INT_MIN || y2 == INT_MIN) && range > 0) {
			y1 = find_ground_level(level::current(), xpos + forward*range, ypos, range+1);
			y2 = find_ground_level(level::current(), xpos - forward*range, ypos, range+1);
			--range;
		}

		if(range == 0) {
			return 0;
		}

		const int dy = y2 - y1;
		const int dx = range*2;
		return (dy*45)/dx;
	}
}

void custom_object::make_draw_color()
{
	if(!draw_color_.get()) {
		draw_color_.reset(new graphics::color_transform(draw_color()));
	}
}

const graphics::color_transform& custom_object::draw_color() const
{
	if(draw_color_.get()) {
		return *draw_color_;
	}

	static const graphics::color_transform white(0xFF, 0xFF, 0xFF, 0xFF);
	return white;
}

game_logic::const_formula_ptr custom_object::get_event_handler(int key) const
{
	if(key < event_handlers_.size()) {
		return event_handlers_[key];
	} else {
		return game_logic::const_formula_ptr();
	}
}

void custom_object::set_event_handler(int key, game_logic::const_formula_ptr f)
{
	if(key >= event_handlers_.size()) {
		event_handlers_.resize(key+1);
	}

	event_handlers_[key] = f;
}

bool custom_object::can_interact_with() const
{
	return can_interact_with_;
}

std::string custom_object::debug_description() const
{
	char buf[128];
	sprintf(buf, "%p", this);
	return type_->id() + " (" + label() + " " + std::string(buf) + ")";
}

namespace {
bool map_variant_entities(variant& v, const std::map<entity_ptr, entity_ptr>& m)
{
	if(v.is_list()) {
		for(int n = 0; n != v.num_elements(); ++n) {
			variant var = v[n];
			if(map_variant_entities(var, m)) {
				std::vector<variant> new_values;
				for(int i = 0; i != n; ++i) {
					new_values.push_back(v[i]);
				}

				new_values.push_back(var);
				for(int i = n+1; i < v.num_elements(); ++i) {
					var = v[i];
					map_variant_entities(var, m);
					new_values.push_back(var);
				}

				v = variant(&new_values);
				return true;
			}
		}
	} else if(v.try_convert<entity>()) {
		entity* e = v.try_convert<entity>();
		std::map<entity_ptr, entity_ptr>::const_iterator i = m.find(entity_ptr(e));
		if(i != m.end()) {
			v = variant(i->second.get());
			return true;
		} else {
			entity_ptr back = e->backup();
			v = variant(back.get());
			return true;
		}
	}

	return false;
}

void do_map_entity(entity_ptr& e, const std::map<entity_ptr, entity_ptr>& m)
{
	if(e) {
		std::map<entity_ptr, entity_ptr>::const_iterator i = m.find(e);
		if(i != m.end()) {
			e = i->second;
		}
	}
}
}

void custom_object::map_entities(const std::map<entity_ptr, entity_ptr>& m)
{
	do_map_entity(last_hit_by_, m);
	do_map_entity(standing_on_, m);
	do_map_entity(parent_, m);

	foreach(variant& v, vars_->values()) {
		map_variant_entities(v, m);
	}

	foreach(variant& v, tmp_vars_->values()) {
		map_variant_entities(v, m);
	}
}

void custom_object::add_particle_system(const std::string& key, const std::string& type)
{
	particle_systems_[key] = type_->get_particle_system_factory(type)->create(*this);
}

void custom_object::remove_particle_system(const std::string& key)
{
	particle_systems_.erase(key);
}

void custom_object::set_text(const std::string& text, const std::string& font, int size, bool centered)
{
	text_.reset(new custom_object_text);
	text_->text = text;
	text_->font = graphical_font::get(font);
	text_->size = size;
	text_->centered = centered;
	text_->alpha = 255;
	ASSERT_LOG(text_->font, "UNKNOWN FONT: " << font);
	text_->dimensions = text_->font->dimensions(text_->text);
}

bool custom_object::boardable_vehicle() const
{
	return type_->is_vehicle() && driver_.get() == NULL;
}

void custom_object::boarded(level& lvl, const entity_ptr& player)
{
	if(!player) {
		return;
	}

	player->board_vehicle();

	if(player->is_human()) {
		playable_custom_object* new_player(new playable_custom_object(*this));
		new_player->driver_ = player;

		lvl.add_player(new_player);

		new_player->get_player_info()->swap_player_state(*player->get_player_info());
		lvl.remove_character(this);
	} else {
		driver_ = player;
		lvl.remove_character(player);
	}
}

void custom_object::unboarded(level& lvl)
{
	if(velocity_x() > 100) {
		driver_->set_face_right(false);
	}

	if(velocity_x() < -100) {
		driver_->set_face_right(true);
	}

	if(is_human()) {
		custom_object* vehicle(new custom_object(*this));
		vehicle->driver_ = entity_ptr();
		lvl.add_character(vehicle);

		lvl.add_player(driver_);

		driver_->unboard_vehicle();

		driver_->get_player_info()->swap_player_state(*get_player_info());
	} else {
		lvl.add_character(driver_);
		driver_->unboard_vehicle();
		driver_ = entity_ptr();
	}
}

void custom_object::board_vehicle()
{
}

void custom_object::unboard_vehicle()
{
}

void custom_object::set_blur(const blur_info* blur)
{
	if(blur) {
		if(blur_) {
			blur_->copy_settings(*blur); 
		} else {
			blur_.reset(new blur_info(*blur));
		}
	} else {
		blur_.reset();
	}
}

void custom_object::set_sound_volume(const int sound_volume)
{
	sound::change_volume(this, sound_volume);
	sound_volume_ = sound_volume;
}

bool custom_object::allow_level_collisions() const
{
	return type_->static_object();
}

void custom_object::set_platform_area(const rect& area)
{
	if(area.w() <= 0 || area.h() <= 0) {
		platform_area_.reset(new rect(area));
		platform_solid_info_ = const_solid_info_ptr();
	} else {
		platform_area_.reset(new rect(area));
		platform_solid_info_ = solid_info::create_platform(area);
	}

	calculate_solid_rect();
}

void custom_object::shift_position(int x, int y)
{
	entity::shift_position(x, y);
	if(standing_on_prev_x_ != INT_MIN) {
		standing_on_prev_x_ += x;
	}

	if(standing_on_prev_y_ != INT_MIN) {
		standing_on_prev_y_ += y;
	}

	if(position_schedule_.get() != NULL) {
		foreach(int& xpos, position_schedule_->x_pos) {
			xpos += x;
		}

		foreach(int& ypos, position_schedule_->y_pos) {
			ypos += y;
		}
	}

	if(activation_area_.get() != NULL) {
		activation_area_.reset(new rect(activation_area_->x() + x,
		                                activation_area_->y() + y,
										activation_area_->w(),
										activation_area_->h()));
	}
}

bool custom_object::appears_at_difficulty(int difficulty) const
{
	return (min_difficulty_ == -1 || difficulty >= min_difficulty_) &&
	       (max_difficulty_ == -1 || difficulty <= max_difficulty_);
}

void custom_object::set_parent(entity_ptr e, const std::string& pivot_point)
{
	parent_ = e;
	parent_pivot_ = pivot_point;

	const point pos = parent_position();
	parent_prev_x_ = pos.x;
	parent_prev_y_ = pos.y;

	if(parent_.get() != NULL) {
		parent_prev_facing_ = parent_->face_right();
	}
}

int custom_object::parent_depth(int cur_depth) const
{
	if(!parent_ || cur_depth > 10) {
		return cur_depth;
	}

	return parent_->parent_depth(cur_depth+1);
}

bool custom_object::editor_force_standing() const
{
	return type_->editor_force_standing();
}

rect custom_object::platform_rect_at(int xpos) const
{
	if(platform_offsets_.empty()) {
		return platform_rect();
	}

	rect area = platform_rect();
	if(xpos < area.x() || xpos >= area.x() + area.w()) {
		return area;
	}

	if(platform_offsets_.size() == 1) {
		return rect(area.x(), area.y() + platform_offsets_[0], area.w(), area.h());
	}

	const int pos = (xpos - area.x())*1024;
	const int seg_width = (area.w()*1024)/(platform_offsets_.size()-1);
	const int segment = pos/seg_width;
	ASSERT_GE(segment, 0);
	ASSERT_LT(segment, platform_offsets_.size()-1);

	const int partial = pos%seg_width;

	const int offset = (partial*platform_offsets_[segment+1] + (seg_width-partial)*platform_offsets_[segment])/seg_width;
	return rect(area.x(), area.y() + offset, area.w(), area.h());
}

bool custom_object::solid_platform() const
{
	return type_->solid_platform();
}

point custom_object::parent_position() const
{
	if(parent_.get() == NULL) {
		return point(0,0);
	}

	return parent_->pivot(parent_pivot_);
}

BENCHMARK(custom_object_spike) {
	static level* lvl = NULL;
	if(!lvl) {	
		lvl = new level("test.cfg");
		static variant v(lvl);
		lvl->finish_loading();
		lvl->set_as_current_level();
	}
	BENCHMARK_LOOP {
		custom_object* obj = new custom_object("chain_base", 0, 0, false);
		variant v(obj);
		obj->handle_event(OBJECT_EVENT_CREATE);
	}
}

int custom_object::events_handled_per_second = 0;

BENCHMARK_ARG(custom_object_get_attr, const std::string& attr)
{
	static custom_object* obj = new custom_object("ant_black", 0, 0, false);
	BENCHMARK_LOOP {
		obj->query_value(attr);
	}
}

BENCHMARK_ARG_CALL(custom_object_get_attr, easy_lookup, "x");
BENCHMARK_ARG_CALL(custom_object_get_attr, hard_lookup, "xxxx");

BENCHMARK_ARG(custom_object_formula, const std::string& f)
{
	static custom_object* obj = new custom_object("ant_black", 0, 0, false);
	const game_logic::formula fml(f, NULL, &custom_object_type::get("ant_black")->callable_definition());
	BENCHMARK_LOOP {
		fml.execute(*obj);
	}
}

BENCHMARK_ARG_CALL_COMMAND_LINE(custom_object_formula);

BENCHMARK_ARG(custom_object_handle_event, const std::string& object_event)
{
	std::string::const_iterator i = std::find(object_event.begin(), object_event.end(), ':');
	ASSERT_LOG(i != object_event.end(), "custom_object_event_handle argument must have a colon seperator: " << object_event);
	std::string obj_type(object_event.begin(), i);
	std::string event_name(i+1, object_event.end());
	static level* lvl = new level("titlescreen.cfg");
	lvl->set_as_current_level();
	static custom_object* obj = new custom_object(obj_type, 0, 0, false);
	obj->set_level(*lvl);
	const int event_id = get_object_event_id(event_name);
	BENCHMARK_LOOP {
		obj->handle_event(event_id);
	}
}

BENCHMARK_ARG_CALL(custom_object_handle_event, ant_non_exist, "ant_black:blahblah");

BENCHMARK_ARG_CALL_COMMAND_LINE(custom_object_handle_event);
