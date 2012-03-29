#include <cassert>
#include <iostream>

#include "asserts.hpp"
#include "collision_utils.hpp"
#include "custom_object.hpp"
#include "custom_object_callable.hpp"
#include "custom_object_functions.hpp"
#include "custom_object_type.hpp"
#include "filesystem.hpp"
#include "formula_constants.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "load_level.hpp"
#include "object_events.hpp"
#include "preferences.hpp"
#include "solid_map.hpp"
#include "sound.hpp"
#include "string_utils.hpp"
#include "surface_cache.hpp"
#include "wml_modify.hpp"
#include "unit_test.hpp"
#include "variant_callable.hpp"
#include "variant_utils.hpp"

namespace {
std::map<std::string, int64_t>& file_mod_times() {
	static std::map<std::string, int64_t> mod_times;
	return mod_times;
}

std::map<std::string, std::string>& object_file_paths() {
	static std::map<std::string, std::string> paths;
	return paths;
}

std::map<std::string, std::string>& prototype_file_paths() {
	static std::map<std::string, std::string> paths;
	return paths;
}

const std::string& object_file_path() {
	if(preferences::load_compiled()) {
		static const std::string value =  "data/compiled/objects/";
		return value;
	} else {
		static const std::string value =  "data/objects/";
		return value;
	}
}

void load_file_paths() {

	//find out the paths to all our files
	sys::get_unique_filenames_under_dir(object_file_path(), &object_file_paths());
	sys::get_unique_filenames_under_dir("data/object_prototypes", &prototype_file_paths());

	for(std::map<std::string, std::string>::const_iterator i = object_file_paths().begin(); i != object_file_paths().end(); ++i) {
		file_mod_times()[i->second] = sys::file_mod_time("./" + i->second);
		std::cerr << "FILE MOD TIME FOR " << i->second << ": " << file_mod_times()[i->second] << "\n";
	}

	for(std::map<std::string, std::string>::const_iterator i = prototype_file_paths().begin(); i != prototype_file_paths().end(); ++i) {
		file_mod_times()[i->second] = sys::file_mod_time(i->second);
	}
}

typedef std::map<std::string, const_custom_object_type_ptr> object_map;

object_map& cache() {
	static object_map instance;
	return instance;
}

namespace {
const std::string BaseStr = "%PROTO%";
}

variant merge_into_prototype(variant prototype_node, variant node)
{
	std::map<variant, variant> result;

	//mapping of animation nodes is kinda complicated: in the
	//prototype there can be one specification of each animation.
	//in objects there can be multiple specifications. Each
	//animation in the object inherits from the specification in
	//the prototype.
	//
	//We are going to build a completely fresh/new set of animations
	//in a vector, and then wipe out all current animations and
	//replace with these from the vector.
	std::vector<variant> animations;
	std::set<std::string> animations_seen;
	foreach(variant anim, node["animation"].as_list()) {
		variant id = anim["id"];
		animations_seen.insert(id.as_string());
		variant proto_anim;
		foreach(variant candidate, prototype_node["animation"].as_list()) {
			if(candidate["id"] == id) {
				proto_anim = candidate;
				break;
			}
		}

		if(proto_anim.is_map()) {
			//the animation is in the prototype, so we merge the
			//object's definition of the animation with the
			//prototype's.
			animations.push_back(proto_anim + anim);
		} else {
			//the animation isn't in the prototype, so just add
			//what is given in the object.
			animations.push_back(anim);
		}
	}

	//now go over the prototype node and add any animations that don't
	//appear in the child.
	foreach(variant anim, prototype_node["animation"].as_list()) {
		if(animations_seen.count(anim["id"].as_string()) == 0) {
			animations.push_back(anim);
		}
	}

	foreach(variant key, prototype_node.get_keys().as_list()) {
		result[key] = prototype_node[key];
	}

	foreach(variant key, node.get_keys().as_list()) {
		variant proto_value = result[key];
		variant value = node[key];

		if(value.is_string()) {
			const std::string& value_str = value.as_string();
			std::string::const_iterator base_itor = std::search(value_str.begin(), value_str.end(), BaseStr.begin(), BaseStr.end());
			if(base_itor != value_str.end()) {
				std::string base_value = "null";
				if(proto_value.is_string()) {
					base_value = proto_value.as_string();
				}
				const std::string s = std::string(value_str.begin(), base_itor) + base_value + std::string(base_itor + BaseStr.size(), value_str.end());
				value = variant(s);
				proto_value = variant();
			}
		}

		result[key] = append_variants(proto_value, value);
	}

	result[variant("animation")] = variant(&animations);

	//any objects which are explicitly merged.
	result[variant("tmp")] = prototype_node["tmp"] + node["tmp"];
	result[variant("vars")] = prototype_node["vars"] + node["vars"];
	result[variant("consts")] = prototype_node["consts"] + node["consts"];
	result[variant("variations")] = prototype_node["variations"] + node["variations"];
	result[variant("properties")] = prototype_node["properties"] + node["properties"];
	result[variant("editor_info")] = prototype_node["editor_info"] + node["editor_info"];

	variant res(&result);
	if(node.get_debug_info()) {
		res.set_debug_info(*node.get_debug_info());
	}

	return res;
}

}

//function which finds if a node has a prototype, and if so, applies the
//prototype to the node.
variant custom_object_type::merge_prototype(variant node)
{
	if(!node.has_key("prototype")) {
		return node;
	}

	std::vector<std::string> protos = util::split(node["prototype"].as_string());

	foreach(const std::string& proto, protos) {
		//look up the object's prototype and merge it in
		std::map<std::string, std::string>::const_iterator path_itor = prototype_file_paths().find(proto + ".cfg");
		ASSERT_LOG(path_itor != prototype_file_paths().end(), "Could not find file for prototype '" << node["prototype"].as_string() << "'");

		variant prototype_node = json::parse_from_file(path_itor->second);
		prototype_node = merge_prototype(prototype_node);
		node = merge_into_prototype(prototype_node, node);
	}
	return node;
}

const std::string* custom_object_type::get_object_path(const std::string& id)
{
	if(object_file_paths().empty()) {
		load_file_paths();
	}

	std::map<std::string, std::string>::const_iterator itor = object_file_paths().find(id);
	if(itor == object_file_paths().end()) {
		return NULL;
	}

	return &itor->second;
}

const_custom_object_type_ptr custom_object_type::get(const std::string& id)
{
	std::string::const_iterator dot_itor = std::find(id.begin(), id.end(), '.');
	if(dot_itor != id.end()) {
		const_custom_object_type_ptr parent = get(std::string(id.begin(), dot_itor));
		if(!parent) {
			return const_custom_object_type_ptr();
		}

		return parent->get_sub_object(std::string(dot_itor+1, id.end()));
	}

	object_map::const_iterator itor = cache().find(id);
	if(itor != cache().end()) {
		return itor->second;
	}

	const_custom_object_type_ptr result(create(id));
	cache()[id] = result;

	//load the object's variations here to avoid pausing the game
	//when an object starts its variation.
	result->load_variations();

	return result;
}

const_custom_object_type_ptr custom_object_type::get_or_die(const std::string& id)
{
	const const_custom_object_type_ptr res = get(id);
	ASSERT_LOG(res.get() != NULL, "UNRECOGNIZED OBJECT TYPE: '" << id << "'");

	return res;
}

const_custom_object_type_ptr custom_object_type::get_sub_object(const std::string& id) const
{
	std::map<std::string, const_custom_object_type_ptr>::const_iterator itor = sub_objects_.find(id);
	if(itor != sub_objects_.end()) {
		return itor->second;
	} else {
		return const_custom_object_type_ptr();
	}
}

custom_object_type_ptr custom_object_type::create(const std::string& id)
{
	if(object_file_paths().empty()) {
		load_file_paths();
	}

	//find the file for the object we are loading.
	std::map<std::string, std::string>::const_iterator path_itor = object_file_paths().find(id + ".cfg");
	ASSERT_LOG(path_itor != object_file_paths().end(), "Could not find file for object '" << id << "'");

	try {
		variant node = merge_prototype(json::parse_from_file(path_itor->second));

		ASSERT_LOG(node["id"].as_string() == id, "IN " << path_itor->second << " OBJECT ID DOES NOT MATCH FILENAME");

		//create the object
		custom_object_type_ptr result(new custom_object_type(node));

		return result;
	} catch(json::parse_error& e) {
		ASSERT_LOG(false, "Error parsing WML for custom object in " << path_itor->second << ": " << e.error_message());
	} catch(graphics::load_image_error&) {
		ASSERT_LOG(false, "Error loading object '" << id << "': could not load needed image");
	}
}

void custom_object_type::invalidate_object(const std::string& id)
{
	cache().erase(id);
}

void custom_object_type::invalidate_all_objects()
{
	cache().clear();
	object_file_paths().clear();
	prototype_file_paths().clear();
}

std::vector<const_custom_object_type_ptr> custom_object_type::get_all()
{
	std::vector<const_custom_object_type_ptr> res;
	std::map<std::string, std::string> file_paths;
	sys::get_unique_filenames_under_dir(object_file_path(), &file_paths);
	for(std::map<std::string, std::string>::const_iterator i = file_paths.begin(); i != file_paths.end(); ++i) {
		const std::string& fname = i->first;
		if(fname.size() < 4 || std::string(fname.end()-4, fname.end()) != ".cfg") {
			continue;
		}

		const std::string id(fname.begin(), fname.end() - 4);
		res.push_back(get(id));
	}

	return res;
}

void custom_object_type::overwrite_frames(custom_object_type* t, custom_object_type* new_obj)
{
	//We keep around leaked frames in case there are still
	//references to them.
	//TODO: make all references to frames use reference-counting
	//pointers and fix this.
	static std::vector<frame_ptr> leaked_frames;

	*t->default_frame_ = *new_obj->default_frame_;
	leaked_frames.push_back(t->default_frame_);

	//clone the frames from the new object into the old one,
	//so if there are any remaining references to the frames they
	//get updated.
	for(frame_map::iterator i = t->frames_.begin(); i != t->frames_.end(); ++i) {
		frame_map::iterator j = new_obj->frames_.find(i->first);
		if(j != new_obj->frames_.end() && i->second.size() == j->second.size()) {
			for(int n = 0; n != i->second.size(); ++n) {
				*i->second[n] = *j->second[n];
			}
		}

		for(int n = 0; n != i->second.size(); ++n) {
			leaked_frames.push_back(i->second[n]);
		}
	}

	for(std::map<std::string, const_custom_object_type_ptr>::iterator i = t->sub_objects_.begin(); i != t->sub_objects_.end(); ++i) {
		std::map<std::string, const_custom_object_type_ptr>::iterator j = new_obj->sub_objects_.find(i->first);
		if(j != new_obj->sub_objects_.end()) {
			overwrite_frames(const_cast<custom_object_type*>(i->second.get()),
			                 const_cast<custom_object_type*>(j->second.get()));
		}
	}

	for(std::map<std::vector<std::string>, const_custom_object_type_ptr>::iterator i = t->variations_cache_.begin(); i != t->variations_cache_.end(); ++i) {
		std::map<std::vector<std::string>, const_custom_object_type_ptr>::iterator j = new_obj->variations_cache_.find(i->first);
		if(j != new_obj->variations_cache_.end()) {
			overwrite_frames(const_cast<custom_object_type*>(i->second.get()),
			                 const_cast<custom_object_type*>(j->second.get()));
		}
	}
}

int custom_object_type::reload_modified_code()
{
	int result = 0;
	for(object_map::iterator i = cache().begin(); i != cache().end(); ++i) {
		const std::string* path = get_object_path(i->first + ".cfg");
		if(!path) {
			continue;
		}

		std::map<std::string, int64_t>::iterator mod_itor = file_mod_times().find(*path);
		const int64_t mod_time = sys::file_mod_time(*path);
		if(mod_time != 0 && mod_itor != file_mod_times().end() && mod_time != mod_itor->second) {
			custom_object_type_ptr new_obj;
			mod_itor->second = mod_time;
			std::cerr << "FILE MODIFIED: " << i->first << " -> " << *path << ": " << mod_time << " VS " << mod_itor->second << "\n";
			
			try {
				const assert_recover_scope scope;
				new_obj = create(i->first);
			} catch(validation_failure_exception& e) {
				std::cerr << "FAILURE TO LOAD\n";
				continue;
			} catch(...) {
				std::cerr << "UNKNOWN FAILURE TO LOAD\n";
				continue;
			}


			custom_object_type* t = const_cast<custom_object_type*>(i->second.get());
			if(new_obj) {
				overwrite_frames(t, new_obj.get());

				*t = *new_obj;
				++result;
			}
		}
	}

	return result;
}


void custom_object_type::init_event_handlers(variant node,
                                             event_handler_map& handlers,
											 game_logic::function_symbol_table* symbols,
											 const event_handler_map* base_handlers)
{
	if(symbols == NULL) {
		symbols = &get_custom_object_functions_symbol_table();
	}

	static custom_object_callable custom_object_definition;

	foreach(const variant_pair& value, node.as_map()) {
		const std::string& key = value.first.as_string();
		if(key.size() > 3 && std::equal(key.begin(), key.begin() + 3, "on_")) {
			const std::string event(key.begin() + 3, key.end());
			const int event_id = get_object_event_id(event);
			if(handlers.size() <= event_id) {
				handlers.resize(event_id+1);
			}

			if(base_handlers && base_handlers->size() > event_id && (*base_handlers)[event_id] && (*base_handlers)[event_id]->str() == value.second.as_string()) {
				handlers[event_id] = (*base_handlers)[event_id];
			} else {
				handlers[event_id] = game_logic::formula::create_optional_formula(value.second, symbols, &custom_object_definition);
			}
		}
	}
}


custom_object_type::custom_object_type(variant node, const custom_object_type* base_type)
  : id_(node["id"].as_string()),
	hitpoints_(node["hitpoints"].as_int(1)),
	timer_frequency_(node["timer_frequency"].as_int(-1)),
	zorder_(node["zorder"].as_int()),
	zsub_order_(node["zsub_order"].as_int()),
	is_human_(node["is_human"].as_bool(false)),
	goes_inactive_only_when_standing_(node["goes_inactive_only_when_standing"].as_bool(false)),
	dies_on_inactive_(node["dies_on_inactive"].as_bool(false)),
	always_active_(node["always_active"].as_bool(false)),
    body_harmful_(node["body_harmful"].as_bool(true)),
    body_passthrough_(node["body_passthrough"].as_bool(false)),
    ignore_collide_(node["ignore_collide"].as_bool(false)),
    object_level_collisions_(node["object_level_collisions"].as_bool(false)),
	surface_friction_(node["surface_friction"].as_int(100)),
	surface_traction_(node["surface_traction"].as_int(100)),
	friction_(node["friction"].as_int()),
	traction_(node["traction"].as_int(1000)),
	traction_in_air_(node["traction_in_air"].as_int(0)),
	traction_in_water_(node["traction_in_water"].as_int(0)),
	respawns_(node["respawns"].as_bool(true)),
	affected_by_currents_(node["affected_by_currents"].as_bool(false)),
	is_vehicle_(node["vehicle"].as_bool(false)),	
	passenger_x_(node["passenger_x"].as_int()),
	passenger_y_(node["passenger_y"].as_int()),
	feet_width_(node["feet_width"].as_int(0)),
	use_image_for_collisions_(node["use_image_for_collisions"].as_bool(false)),
	static_object_(node["static_object"].as_bool(use_image_for_collisions_)),
	has_feet_(node["has_feet"].as_bool(true) && static_object_ == false),
	adjust_feet_on_animation_change_(node["adjust_feet_on_animation_change"].as_bool(true)),
	teleport_offset_x_(node["teleport_offset_x"].as_int()),
	teleport_offset_y_(node["teleport_offset_y"].as_int()),
	no_move_to_standing_(node["no_move_to_standing"].as_bool()),
	reverse_global_vertical_zordering_(node["reverse_global_vertical_zordering"].as_bool(false)),
	serializable_(node["serializable"].as_bool(true)),
	solid_(solid_info::create(node)),
	platform_(solid_info::create_platform(node)),
	solid_platform_(node["solid_platform"].as_bool(false)),
	has_solid_(solid_ || use_image_for_collisions_),
	solid_dimensions_(has_solid_ || platform_ ? 0xFFFFFFFF : 0),
	collide_dimensions_(0xFFFFFFFF),
	weak_solid_dimensions_(has_solid_ || platform_ ? 0xFFFFFFFF : 0),
	weak_collide_dimensions_(0xFFFFFFFF),
	activation_border_(node["activation_border"].as_int(100)),
	editor_force_standing_(node["editor_force_standing"].as_bool(false)),
	hidden_in_game_(node["hidden_in_game"].as_bool(false)),
	platform_offsets_(node["platform_offsets"].as_list_int_optional())
{
	std::cerr << "SOLID FOR " << id_ << " " << (solid_ ? "YES" : "NO") << "\n";
	if(node.has_key("editor_info")) {
		editor_info_.reset(new editor_entity_info(node["editor_info"]));
	}

	if(node.has_key("preload_sounds")) {
		//Pre-load any sounds that should be present when we create
		//this object type.
		foreach(std::string sound, util::split(node["preload_sounds"].as_string())) {
			sound::preload(sound);
		}
	}

	const bool is_variation = base_type != NULL;

	//make it so any formula has these constants defined.
	const game_logic::constants_loader scope_consts(node["consts"]);

	//if some constants change from base to variation, then we have to
	//re-parse all formulas.
	if(scope_consts.same_as_base() == false) {
		base_type = NULL;
	}

	if(node.has_key("solid_dimensions")) {
		weak_solid_dimensions_ = solid_dimensions_ = 0;
		foreach(std::string key, node["solid_dimensions"].as_list_string()) {
			if(key.empty() == false && key[0] == '~') {
				key = std::string(key.begin()+1, key.end());
				weak_solid_dimensions_ |= (1 << get_solid_dimension_id(key));
			} else {
				solid_dimensions_ |= (1 << get_solid_dimension_id(key));
			}
		}

		weak_solid_dimensions_ |= solid_dimensions_;
	}

	if(node.has_key("collide_dimensions")) {
		weak_collide_dimensions_ = collide_dimensions_ = 0;
		foreach(std::string key, node["collide_dimensions"].as_list_string()) {
			if(key.empty() == false && key[0] == '~') {
				key = std::string(key.begin()+1, key.end());
				weak_collide_dimensions_ |= (1 << get_solid_dimension_id(key));
			} else {
				collide_dimensions_ |= (1 << get_solid_dimension_id(key));
			}
		}

		weak_collide_dimensions_ |= collide_dimensions_;
	}

	foreach(variant anim, node["animation"].as_list()) {
		boost::shared_ptr<frame> f;
		try {
			f.reset(new frame(anim));
		} catch(frame::error&) {
			ASSERT_LOG(false, "ERROR LOADING FRAME IN OBJECT '" << id_ << "'");
		}

		if(use_image_for_collisions_) {
			f->set_image_as_solid();
		}

		if(f->solid()) {
			has_solid_ = true;
		}

		frames_[anim["id"].as_string()].push_back(f);
		const int duplicates = anim["duplicates"].as_int();
		if(duplicates > 1) {
			for(int n = 1; n != duplicates; ++n) {
				frames_[anim["id"].as_string()].push_back(f);
			}
		}
		if(!default_frame_) {
			default_frame_ = f;
		}
	}

	ASSERT_LOG(default_frame_, "OBJECT " << node.write_json() << " NO ANIMATIONS FOR OBJECT '" << id_ << "'");

	std::vector<variant> available_frames;
	for(frame_map::const_iterator i = frames_.begin(); i != frames_.end(); ++i) {
		available_frames.push_back(variant(i->first));
	}

	available_frames_ = variant(&available_frames);

	mass_ = node["mass"].as_int(default_frame_->collide_w() * default_frame_->collide_h());
	
	foreach(variant child, node["child"].as_list()) {
		const std::string& child_id = child["child_id"].as_string();
		children_[child_id] = child;
	}

	assert(default_frame_);

	next_animation_formula_ = game_logic::formula::create_optional_formula(node["next_animation"], function_symbols());

	foreach(variant particle_node, node["particle_system"].as_list()) {
		particle_factories_[particle_node["id"].as_string()] = particle_system_factory::create_factory(particle_node);
	}

	if(!is_variation) {
		foreach(variant object_node, node["object_type"].as_list()) {
			variant merged = merge_prototype(object_node);
			custom_object_type* type = new custom_object_type(merged);
			type->id_ = id_ + "." + type->id_;
			//std::cerr << "MERGED PROTOTYPE FOR " << type->id_ << ": " << merged.write_json() << "\n";
			sub_objects_[object_node["id"].as_string()].reset(type);
		}
	}

	if(node.has_key("parallax_scale_x") || node.has_key("parallax_scale_y")) {
		parallax_scale_millis_.reset(new std::pair<int, int>(node["parallax_scale_x"].as_int(1000), node["parallax_scale_y"].as_int(1000)));
	}
	
	variant vars = node["vars"];
	if(vars.is_null() == false) {
		std::vector<std::string> var_str;
		foreach(variant key, vars.get_keys().as_list()) {
			variables_[key.as_string()] = vars[key];
			var_str.push_back(key.as_string());
		}

		if(!var_str. empty()) {
			game_logic::formula_callable_definition::entry* entry = callable_definition_.get_entry(CUSTOM_OBJECT_VARS);
			ASSERT_LOG(entry != NULL, "CANNOT FIND VARS ENTRY IN OBJECT");
			entry->type_definition_holder = game_logic::create_formula_callable_definition(&var_str[0], &var_str[0] + var_str.size());
			entry->type_definition = entry->type_definition_holder.get();
		}
	}

	variant tmp_vars = node["tmp_vars"];
	if(tmp_vars.is_null() == false) {
		std::vector<std::string> var_str;
		foreach(variant key, tmp_vars.get_keys().as_list()) {
			variables_[key.as_string()] = tmp_vars[key];
			var_str.push_back(key.as_string());
		}

		if(!var_str.empty()) {
			game_logic::formula_callable_definition::entry* entry = callable_definition_.get_entry(CUSTOM_OBJECT_TMP);
			ASSERT_LOG(entry != NULL, "CANNOT FIND TMP ENTRY IN OBJECT");
			entry->type_definition_holder = game_logic::create_formula_callable_definition(&var_str[0], &var_str[0] + var_str.size());
			entry->type_definition = entry->type_definition_holder.get();
		}
	}

	//std::cerr << "TMP_VARIABLES: '" << id_ << "' -> " << tmp_variables_.size() << "\n";

	consts_.reset(new game_logic::map_formula_callable);
	variant consts = node["consts"];
	if(consts.is_null() == false) {
		foreach(variant key, consts.get_keys().as_list()) {
			consts_->add(key.as_string(), consts[key]);
		}
	}

	if(node.has_key("tags")) {
		const std::vector<std::string> tags = util::split(node["tags"].as_string());
		foreach(const std::string& tag, tags) {
			tags_[tag] = variant(1);
		}
	}

	foreach(variant properties_node, node["properties"].as_list()) {
		foreach(variant key, properties_node.get_keys().as_list()) {
			const std::string& k = key.as_string();
			variant value = properties_node[key];
			if(value.is_string()) {
				properties_[k] = game_logic::formula::create_optional_formula(value, function_symbols(), &callable_definition_);
			} else {
				const_properties_[k] = value;
			}
		}
	}

	variant variations = node["variations"];
	if(variations.is_null() == false) {
		foreach(const variant_pair& v, variations.as_map()) {
			variations_[v.first.as_string()] = game_logic::formula::create_optional_formula(v.second, &get_custom_object_functions_symbol_table());
		}
		
		node_ = node;
	}

	game_logic::register_formula_callable_definition("object_type", &callable_definition_);

	if(base_type) {
		//if we're a variation, just get the functions from our base type.
		//variations can't define new functions.
		object_functions_ = base_type->object_functions_;
	} else if(node.has_key("functions")) {
		object_functions_.reset(new game_logic::function_symbol_table);
		object_functions_->set_backup(&get_custom_object_functions_symbol_table());
		game_logic::formula f(node["functions"], object_functions_.get());
	}
	init_event_handlers(node, event_handlers_, function_symbols(), base_type ? &base_type->event_handlers_ : NULL);
}

custom_object_type::~custom_object_type()
{
}

const frame& custom_object_type::default_frame() const
{
	return *default_frame_;
}

const frame& custom_object_type::get_frame(const std::string& key) const
{
	frame_map::const_iterator itor = frames_.find(key);
	if(itor == frames_.end() || itor->second.empty()) {
		ASSERT_LOG(key == "normal", "UNKNOWN FRAME " << key << " IN " << id_);
		return default_frame();
	} else {
		if(itor->second.size() == 1) {
			return *itor->second.front().get();
		} else {
			return *itor->second[rand()%itor->second.size()].get();
		}
	}
}

game_logic::const_formula_ptr custom_object_type::get_event_handler(int event) const
{
	if(event >= event_handlers_.size()) {
		return game_logic::const_formula_ptr();
	} else {
		return event_handlers_[event];
	}
}

const_particle_system_factory_ptr custom_object_type::get_particle_system_factory(const std::string& id) const
{
	std::map<std::string, const_particle_system_factory_ptr>::const_iterator i = particle_factories_.find(id);
	ASSERT_LOG(i != particle_factories_.end(), "Unknown particle system type in " << id_ << ": " << id);
	return i->second;
}

game_logic::function_symbol_table* custom_object_type::function_symbols() const
{
	if(object_functions_) {
		return object_functions_.get();
	} else {
		return &get_custom_object_functions_symbol_table();
	}
}

namespace {
void execute_variation_command(variant cmd, level& lvl, custom_object& obj)
{
	if(cmd.is_list()) {
		foreach(variant c, cmd.as_list()) {
			execute_variation_command(c, lvl, obj);
		}
	} else if(cmd.try_convert<entity_command_callable>()) {
		cmd.try_convert<entity_command_callable>()->execute(lvl, obj);
	} else if(cmd.try_convert<custom_object_command_callable>()) {
		cmd.try_convert<custom_object_command_callable>()->execute(lvl, obj);
	}
}
}

const_custom_object_type_ptr custom_object_type::get_variation(const std::vector<std::string>& variations) const
{
	ASSERT_LOG(node_.is_null() == false, "tried to set variation in object " << id_ << " which has no variations");
	if(!level::current_ptr()) {
		//hack to get a current level -- load a basic empty level
		static boost::intrusive_ptr<level> lvl(load_level("empty.cfg"));
		lvl->finish_loading();
		lvl->set_as_current_level();
	}

	const_custom_object_type_ptr& result = variations_cache_[variations];
	if(!result) {
		variant node = node_;
		boost::intrusive_ptr<custom_object> obj(new custom_object(id_, 0, 0, true));

		boost::intrusive_ptr<game_logic::map_formula_callable> callable(new game_logic::map_formula_callable);
		callable->add("doc", variant(variant_callable::create(&node)));

		foreach(const std::string& v, variations) {
			std::map<std::string, game_logic::const_formula_ptr>::const_iterator var_itor = variations_.find(v);
			ASSERT_LOG(var_itor != variations_.end(), "COULD NOT FIND VARIATION " << v << " IN " << id_);

			variant cmd = var_itor->second->execute(*callable);

			execute_variation_command(cmd, level::current(), *obj);
		}

		std::cerr << "VARIATION MODIFICATION: BEFORE\n---\n" << node_.write_json() << "---\nAFTER\n---\n" << node.write_json() << "\n---\n";

		//set our constants so the variation can decide whether it needs
		//to re-parse formulas or not.
		const game_logic::constants_loader scope_consts(node_["consts"]);

		result.reset(new custom_object_type(node, this));
	}

	return result;
}

void custom_object_type::load_variations() const
{
	if(node_.is_null() || variations_.empty() || !node_.has_key("load_variations")) {
		return;
	}

	const std::vector<std::string> variations_to_load = util::split(node_["load_variations"].as_string());
	foreach(const std::string& v, variations_to_load) {
		get_variation(std::vector<std::string>(1, v));
	}
}

#include "texture.hpp"
#include "surface_cache.hpp"

BENCHMARK(custom_object_type_load)
{
	static std::map<std::string,std::string> file_paths;
	if(file_paths.empty()) {
		sys::get_unique_filenames_under_dir("data/objects", &file_paths);
	}

	BENCHMARK_LOOP {
		for(std::map<std::string,std::string>::const_iterator i = file_paths.begin(); i != file_paths.end(); ++i) {
			if(i->first.size() > 4 && std::equal(i->first.end()-4, i->first.end(), ".cfg")) {
				custom_object_type::create(std::string(i->first.begin(), i->first.end()-4));
			}
		}
		graphics::surface_cache::clear();
		graphics::texture::clear_textures();
	}
}


BENCHMARK(custom_object_type_frogatto_load)
{
	BENCHMARK_LOOP {
		custom_object_type::create("frogatto_playable");
		graphics::texture::clear_textures();
		graphics::surface_cache::clear();
	}
}

UTILITY(object_definition)
{
	foreach(const std::string& arg, args) {
		const_custom_object_type_ptr obj = custom_object_type::get(arg);
		ASSERT_LOG(obj.get() != NULL, "NO OBJECT FOUND: " << arg);

		const std::string* fname = custom_object_type::get_object_path(arg + ".cfg");
		ASSERT_LOG(fname != NULL, "NO OBJECT FILE FOUND: " << arg);

		const variant node = custom_object_type::merge_prototype(json::parse_from_file(*fname));

		std::cout << "OBJECT " << arg << "\n---\n" << node.write_json(true) << "\n---\n";
	}
}
