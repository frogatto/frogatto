#include <cassert>
#include <iostream>

#include <boost/bind.hpp>

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
#include "module.hpp"
#include "object_events.hpp"
#include "preferences.hpp"
#include "solid_map.hpp"
#include "sound.hpp"
#include "string_utils.hpp"
#include "surface_cache.hpp"
#include "unit_test.hpp"
#include "variant_callable.hpp"
#include "variant_utils.hpp"

std::map<std::string, std::string>& prototype_file_paths() {
	static std::map<std::string, std::string> paths;
	return paths;
}

namespace {

std::map<std::string, std::string>& object_file_paths() {
	static std::map<std::string, std::string> paths;
	return paths;
}

const std::string& object_file_path() {
	if(preferences::load_compiled()) {
		static const std::string value =  "data/compiled/objects";
		return value;
	} else {
		static const std::string value =  "data/objects";
		return value;
	}
}

void load_file_paths() {

	//find out the paths to all our files
	module::get_unique_filenames_under_dir(object_file_path(), &object_file_paths());
	module::get_unique_filenames_under_dir("data/object_prototypes", &::prototype_file_paths());
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

		if(value.is_null()) {
			//An explicit null in the object will kill the
			//attribute entirely.
			result[key] = variant();
			continue;
		}

		if(key.as_string().size() > 3 && std::equal(key.as_string().begin(), key.as_string().begin() + 3, "on_")) {
			if(proto_value.is_string()) {
				std::string k = key.as_string();
				const std::string proto_event_key = "on_" + prototype_node["id"].as_string() + "_PROTO_" + std::string(k.begin() + 3, k.end());
				result[variant(proto_event_key)] = proto_value;
			}
		}

		if(value.is_string()) {
			const std::string& value_str = value.as_string();
			std::string::const_iterator base_itor = std::search(value_str.begin(), value_str.end(), BaseStr.begin(), BaseStr.end());
			if(base_itor != value_str.end()) {
				const variant::debug_info* info = value.get_debug_info();
				std::string base_value = "null";
				if(proto_value.is_string()) {
					base_value = proto_value.as_string();
				}
				const std::string s = std::string(value_str.begin(), base_itor) + base_value + std::string(base_itor + BaseStr.size(), value_str.end());
				value = variant(s);
				proto_value = variant();

				if(info) {
					value.set_debug_info(*info);
				}
			}
		}

		result[key] = append_variants(proto_value, value);
	}

	std::vector<variant> functions;
	variant proto_fn = prototype_node["functions"];
	if(proto_fn.is_string()) {
		functions.push_back(proto_fn);
	} else if(proto_fn.is_list()) {
		foreach(variant v, proto_fn.as_list()) {
			functions.push_back(v);
		}
	}

	variant fn = node["functions"];
	if(fn.is_string()) {
		functions.push_back(fn);
	} else if(fn.is_list()) {
		foreach(variant v, fn.as_list()) {
			functions.push_back(v);
		}
	}

	if(!functions.empty()) {
		result[variant("functions")] = variant(&functions);
	}

	result[variant("animation")] = variant(&animations);

	//any objects which are explicitly merged.
	result[variant("tmp")] = prototype_node["tmp"] + node["tmp"];
	result[variant("vars")] = prototype_node["vars"] + node["vars"];
	result[variant("consts")] = prototype_node["consts"] + node["consts"];
	result[variant("variations")] = prototype_node["variations"] + node["variations"];

	const variant editor_info_a = prototype_node["editor_info"];
	const variant editor_info_b = node["editor_info"];
	result[variant("editor_info")] = editor_info_a + editor_info_b;
	if(editor_info_a.is_map() && editor_info_b.is_map() &&
	   editor_info_a["var"].is_list() && editor_info_b["var"].is_list()) {
		std::map<variant, variant> vars_map;
		std::vector<variant> items = editor_info_a["var"].as_list();
		std::vector<variant> items2 = editor_info_b["var"].as_list();
		items.insert(items.end(), items2.begin(), items2.end());
		foreach(const variant& v, items) {
			variant name = v["name"];
			variant enum_value;
			if(vars_map.count(name)) {
				if(vars_map[name]["enum_values"].is_list() && v["enum_values"].is_list()) {
					std::vector<variant> e = vars_map[name]["enum_values"].as_list();
					foreach(variant item, v["enum_values"].as_list()) {
						if(std::count(e.begin(), e.end(), item) == 0) {
							e.push_back(item);
						}
					}

					enum_value = variant(&e);
				}

				vars_map[name] = vars_map[name] + v;
				if(enum_value.is_null() == false) {
					vars_map[name].add_attr(variant("enum_values"), enum_value);
				}
			} else {
				vars_map[name] = v;
			}
		}

		std::vector<variant> v;
		for(std::map<variant,variant>::const_iterator i = vars_map.begin(); i != vars_map.end(); ++i) {
			v.push_back(i->second);
		}

		variant vars = variant(&v);
		result[variant("editor_info")].add_attr(variant("var"), vars);
	}

	variant proto_properties = prototype_node["properties"];
	variant node_properties = node["properties"];
	variant properties = proto_properties + node_properties;
	if(proto_properties.is_map() && node_properties.is_map()) {
		foreach(const variant_pair& p, proto_properties.as_map()) {
			if(node_properties.has_key(p.first)) {
				const std::string proto_id = prototype_node["id"].as_string() + "_" + p.first.as_string();
				properties.add_attr(variant(proto_id), p.second);
			}
		}
	}
	result[variant("properties")] = properties;

	variant res(&result);
	if(node.get_debug_info()) {
		res.set_debug_info(*node.get_debug_info());
	}

	return res;
}

}

namespace customobjecttype {
void reload_file_paths() {
	custom_object_type::invalidate_all_objects();
	load_file_paths();
}
}


//function which finds if a node has a prototype, and if so, applies the
//prototype to the node.
variant custom_object_type::merge_prototype(variant node, std::vector<std::string>* proto_paths)
{
	if(!node.has_key("prototype")) {
		return node;
	}

	std::vector<std::string> protos = node["prototype"].as_list_string();

	foreach(const std::string& proto, protos) {
		//look up the object's prototype and merge it in
		std::map<std::string, std::string>::const_iterator path_itor = module::find(::prototype_file_paths(), proto + ".cfg");
		ASSERT_LOG(path_itor != ::prototype_file_paths().end(), "Could not find file for prototype '" << proto << "'");

		variant prototype_node = json::parse_from_file(path_itor->second);
		ASSERT_LOG(prototype_node["id"].as_string() == proto, "PROTOTYPE NODE FOR " << proto << " DOES NOT SPECIFY AN ACCURATE id FIELD");
		if(proto_paths) {
			proto_paths->push_back(path_itor->second);
		}
		prototype_node = merge_prototype(prototype_node, proto_paths);
		node = merge_into_prototype(prototype_node, node);
	}
	return node;
}

const std::string* custom_object_type::get_object_path(const std::string& id)
{
	if(object_file_paths().empty()) {
		load_file_paths();
	}

	std::map<std::string, std::string>::const_iterator itor = module::find(object_file_paths(), id);
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

	object_map::const_iterator itor = cache().find(module::get_id(id));
	if(itor != cache().end()) {
		return itor->second;
	}

	const_custom_object_type_ptr result(create(id));
	cache()[module::get_id(id)] = result;

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
	return recreate(id, NULL);
}

namespace {
std::map<std::string, std::vector<std::string> > object_prototype_paths;
}

custom_object_type_ptr custom_object_type::recreate(const std::string& id,
                                             const custom_object_type* old_type)
{
	std::cerr << "CREATE OBJ: "<< id << "\n";
	if(object_file_paths().empty()) {
		load_file_paths();
	}

	//find the file for the object we are loading.
	std::map<std::string, std::string>::const_iterator path_itor = module::find(object_file_paths(), id + ".cfg");
	ASSERT_LOG(path_itor != object_file_paths().end(), "Could not find file for object '" << id << "'");

	try {
		std::vector<std::string> proto_paths;
		variant node = merge_prototype(json::parse_from_file(path_itor->second), &proto_paths);

		ASSERT_LOG(node["id"].as_string() == module::get_id(id), "IN " << path_itor->second << " OBJECT ID DOES NOT MATCH FILENAME");
		
		//create the object
		custom_object_type_ptr result(new custom_object_type(node, NULL, old_type));
		object_prototype_paths[id] = proto_paths;

		return result;
	} catch(json::parse_error& e) {
		ASSERT_LOG(false, "Error parsing WML for custom object '" << id << "' in '" << path_itor->second << "': '" << e.error_message() << "'");
	} catch(graphics::load_image_error&) {
		ASSERT_LOG(false, "Error loading object '" << id << "': could not load needed image");
	}
	// We never get here, but this stops a compiler warning.
	return custom_object_type_ptr();
}

void custom_object_type::invalidate_object(const std::string& id)
{
	cache().erase(module::get_id(id));
}

void custom_object_type::invalidate_all_objects()
{
	cache().clear();
	object_file_paths().clear();
	::prototype_file_paths().clear();
}

std::vector<std::string> custom_object_type::get_all_ids()
{
	std::vector<std::string> res;
	std::map<std::string, std::string> file_paths;
	module::get_unique_filenames_under_dir(object_file_path(), &file_paths);
	for(std::map<std::string, std::string>::const_iterator i = file_paths.begin(); i != file_paths.end(); ++i) {
		const std::string& fname = i->first;
		if(fname.size() < 4 || std::string(fname.end()-4, fname.end()) != ".cfg") {
			continue;
		}

		const std::string id(fname.begin(), fname.end() - 4);
		res.push_back(id);
	}

	return res;
}

std::map<std::string,custom_object_type::EditorSummary> custom_object_type::get_editor_categories()
{
	const std::string path = std::string(preferences::user_data_path()) + "/editor_cache.cfg";
	variant cache, proto_cache;
	if(sys::file_exists(path)) {
		try {
			cache = json::parse(sys::read_file(path), json::JSON_NO_PREPROCESSOR);
			proto_cache = cache["prototype_info"];
		} catch(...) {
		}
	}

	std::map<std::string, bool> proto_status;

	std::map<variant, variant> items, proto_info;
	foreach(const std::string& id, get_all_ids()) {
		variant info;
		const std::string* path = get_object_path(id + ".cfg");
		if(path == NULL) {
			fprintf(stderr, "NO FILE FOR OBJECT '%s'\n", id.c_str());
		}

		const int mod_time = static_cast<int>(sys::file_mod_time(*path));
		if(cache.is_map() && cache.has_key(id) && cache[id]["mod"].as_int() == mod_time) {
			info = cache[id];
			foreach(const std::string& p, info["prototype_paths"].as_list_string()) {
				if(!proto_status.count(p)) {
					const int t = static_cast<int>(sys::file_mod_time(p));
					proto_info[variant(p)] = variant(t);
					proto_status[p] = t == proto_cache[p].as_int();
				}

				if(!proto_status[p]) {
					info = variant();
					break;
				}
			}
		}

		if(info.is_null()) {
			std::vector<std::string> proto_paths;
			variant node = merge_prototype(json::parse_from_file(*path), &proto_paths);
			std::map<variant,variant> summary;
			summary[variant("mod")] = variant(mod_time);
			std::vector<variant> proto_paths_v;
			foreach(const std::string& s, proto_paths) {
				proto_paths_v.push_back(variant(s));
			}

			summary[variant("prototype_paths")] = variant(&proto_paths_v);

			if(node["animation"].is_list()) {
				summary[variant("animation")] = node["animation"][0];
			} else if(node["animation"].is_map()) {
				summary[variant("animation")] = node["animation"];
			}

			if(node["editor_info"].is_map()) {
				summary[variant("category")] = node["editor_info"]["category"];
				if(node["editor_info"]["help"].is_string()) {
					summary[variant("help")] = node["editor_info"]["help"];
				}
			}

			info = variant(&summary);
		}

		items[variant(id)] = info;
	}

	std::map<std::string,EditorSummary> m;
	for(std::map<variant,variant>::const_iterator i = items.begin(); i != items.end(); ++i) {
		if(i->second.has_key("category")) {
			EditorSummary& summary = m[i->first.as_string()];
			summary.category = i->second["category"].as_string();
			if(i->second["help"].is_string()) {
				summary.help = i->second["help"].as_string();
			}
			summary.first_frame = i->second["animation"];
		}
	}

	items[variant("prototype_info")] = variant(&proto_info);

	const variant result = variant(&items);
	sys::write_file(path, result.write_json());

	return m;
}

std::vector<const_custom_object_type_ptr> custom_object_type::get_all()
{
	std::vector<const_custom_object_type_ptr> res;
	foreach(const std::string& id, get_all_ids()) {
		res.push_back(get(id));
	}

	return res;
}

#ifndef NO_EDITOR
namespace {
std::set<std::string> listening_for_files, files_updated;

void on_object_file_updated(std::string path)
{
	files_updated.insert(path);
}
}

int custom_object_type::reload_modified_code()
{
	static int prev_nitems = 0;
	const int nitems = cache().size();
	if(prev_nitems == nitems && files_updated.empty()) {
		return 0;
	}

	prev_nitems = nitems;

	std::set<std::string> error_paths;

	int result = 0;
	for(object_map::iterator i = cache().begin(); i != cache().end(); ++i) {

		const std::string* path = get_object_path(i->first + ".cfg");

		if(!path) {
			continue;
		}

		if(listening_for_files.count(*path) == 0) {
			sys::notify_on_file_modification(*path, boost::bind(on_object_file_updated, *path));
			listening_for_files.insert(*path);
		}

		if(files_updated.count(*path)) {
			try {
				reload_object(i->first);
				++result;
			} catch(...) {
				error_paths.insert(*path);
			}
		}
	}

	files_updated = error_paths;

	return result;
}
#endif // NO_EDITOR

void custom_object_type::set_file_contents(const std::string& file_path, const std::string& contents)
{
	json::set_file_contents(file_path, contents);
	for(object_map::iterator i = cache().begin(); i != cache().end(); ++i) {
		const std::vector<std::string>& proto_paths = object_prototype_paths[i->first];
		const std::string* path = get_object_path(i->first + ".cfg");
		if(path && *path == file_path || std::count(proto_paths.begin(), proto_paths.end(), file_path)) {
			std::cerr << "RELOAD OBJECT: " << i->first << " -> " << *path << "\n";
			reload_object(i->first);
		}
	}
}

namespace {
int g_num_object_reloads = 0;
}

void custom_object_type::reload_object(const std::string& type)
{
	object_map::iterator itor = cache().find(module::get_id(type));
	ASSERT_LOG(itor != cache().end(), "COULD NOT RELOAD OBJECT " << type);
	
	const_custom_object_type_ptr old_obj = itor->second;

	custom_object_type_ptr new_obj;
	
	const int begin = SDL_GetTicks();
	{
		const assert_recover_scope scope;
		new_obj = recreate(type, old_obj.get());
		std::cerr << "RELOADED OBJECT IN " << (SDL_GetTicks() - begin) << "ms\n";
	}

	if(!new_obj) {
		return;
	}

	const int start = SDL_GetTicks();
	foreach(custom_object* obj, custom_object::get_all(old_obj->id())) {
		obj->update_type(old_obj, new_obj);
	}

	for(std::map<std::string, const_custom_object_type_ptr>::const_iterator i = old_obj->sub_objects_.begin(); i != old_obj->sub_objects_.end(); ++i) {
		std::map<std::string, const_custom_object_type_ptr>::const_iterator j = new_obj->sub_objects_.find(i->first);
		if(j != new_obj->sub_objects_.end() && i->second != j->second) {
			foreach(custom_object* obj, custom_object::get_all(i->second->id())) {
				obj->update_type(i->second, j->second);
			}
		}
	}

	const int end = SDL_GetTicks();
	std::cerr << "UPDATED " << custom_object::get_all(old_obj->id()).size() << " OBJECTS IN " << (end - start) << "ms\n";

	itor->second = new_obj;

	++g_num_object_reloads;
}

int custom_object_type::num_object_reloads()
{
	return g_num_object_reloads;
}

void custom_object_type::init_event_handlers(variant node,
                                             event_handler_map& handlers,
											 game_logic::function_symbol_table* symbols,
											 const event_handler_map* base_handlers) const
{
	if(symbols == NULL) {
		symbols = &get_custom_object_functions_symbol_table();
	}

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
				handlers[event_id] = game_logic::formula::create_optional_formula(value.second, symbols, &callable_definition_);
			}
		}
	}
}

custom_object_type::custom_object_type(variant node, const custom_object_type* base_type, const custom_object_type* old_type)
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
	collides_with_level_(node["collides_with_level"].as_bool(true)),
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
	weak_solid_dimensions_(has_solid_ || platform_ || node["has_platform"].as_bool(false) ? 0xFFFFFFFF : 0),
	weak_collide_dimensions_(0xFFFFFFFF),
	activation_border_(node["activation_border"].as_int(100)),
	editor_force_standing_(node["editor_force_standing"].as_bool(false)),
	hidden_in_game_(node["hidden_in_game"].as_bool(false)),
	platform_offsets_(node["platform_offsets"].as_list_int_optional()),
	slot_properties_base_(-1), 
	use_absolute_screen_coordinates_(node["use_absolute_screen_coordinates"].as_bool(false))
{
#ifndef NO_EDITOR
	if(node.has_key("editor_info")) {
		editor_info_.reset(new editor_entity_info(node["editor_info"]));
	}
#endif // !NO_EDITOR

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

	ASSERT_LOG(default_frame_, "OBJECT " << id_ << " NO ANIMATIONS FOR OBJECT: " << node.write_json() << "'");

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
			std::string sub_key = object_node["id"].as_string();

			if(old_type && old_type->sub_objects_.count(sub_key) &&
			   old_type->sub_objects_.find(sub_key)->second->node_ == merged) {
				//We are recreating this object, and the sub object node
				//hasn't changed at all, so just reuse the same sub object.
				sub_objects_[sub_key] = old_type->sub_objects_.find(sub_key)->second;
			} else {
				custom_object_type* type = new custom_object_type(merged);
				type->id_ = id_ + "." + type->id_;
				if(old_type && type->node_.is_null()){
					type->node_ = merged;
				}
			//std::cerr << "MERGED PROTOTYPE FOR " << type->id_ << ": " << merged.write_json() << "\n";
				sub_objects_[sub_key].reset(type);
			}
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

	variant tmp_vars = node["tmp"];
	if(tmp_vars.is_null() == false) {
		std::vector<std::string> var_str;
		foreach(variant key, tmp_vars.get_keys().as_list()) {
			tmp_variables_[key.as_string()] = tmp_vars[key];
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

	slot_properties_base_ = callable_definition_.num_slots();
	foreach(variant properties_node, node["properties"].as_list()) {
		foreach(variant key, properties_node.get_keys().as_list()) {
			const std::string& k = key.as_string();
			callable_definition_.add_property(k);
			variant value = properties_node[key];
			property_entry& entry = properties_[k];
			if(value.is_string()) {
				entry.getter = game_logic::formula::create_optional_formula(value, function_symbols(), &callable_definition_);
			} else if(value.is_map()) {
				entry.getter = game_logic::formula::create_optional_formula(value["get"], function_symbols(), &callable_definition_);
				entry.setter = game_logic::formula::create_optional_formula(value["set"], function_symbols(), &callable_definition_);
			} else {
				entry.const_value.reset(new variant(value));
			}

			if(entry.getter) {
				variant v;
				if(entry.getter->evaluates_to_constant(v)) {
					entry.getter.reset();
					entry.const_value.reset(new variant(v));
				}
			}
			slot_properties_.push_back(entry);
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

#if defined(USE_GLES2)
	if(node.has_key("shader")) {
		shader_.reset(new gles2::shader_program(node["shader"]));
	}

	if(node.has_key("effects")) {
		effects_.clear();
		for(size_t n = 0; n < node["effects"].num_elements(); ++n) {
			effects_.push_back(gles2::shader_ptr(new gles2::shader_program(node["effects"][n])));
		}
	}
#endif

	if(base_type) {
		//if we're a variation, just get the functions from our base type.
		//variations can't define new functions.
		object_functions_ = base_type->object_functions_;
	} else if(node.has_key("functions")) {
		object_functions_.reset(new game_logic::function_symbol_table);
		object_functions_->set_backup(&get_custom_object_functions_symbol_table());
		const variant fn = node["functions"];
		if(fn.is_string()) {
			game_logic::formula f(fn, object_functions_.get());
		} else if(fn.is_list()) {
			for(int n = 0; n != fn.num_elements(); ++n) {
				game_logic::formula f(fn[n], object_functions_.get());
			}
		}
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
void execute_variation_command(variant cmd, game_logic::formula_callable& obj)
{
	if(cmd.is_list()) {
		foreach(variant c, cmd.as_list()) {
			execute_variation_command(c, obj);
		}
	} else if(cmd.try_convert<game_logic::command_callable>()) {
		cmd.try_convert<game_logic::command_callable>()->execute(obj);
	}
}
}

const_custom_object_type_ptr custom_object_type::get_variation(const std::vector<std::string>& variations) const
{
	ASSERT_LOG(node_.is_null() == false, "tried to set variation in object " << id_ << " which has no variations");

	const_custom_object_type_ptr& result = variations_cache_[variations];
	if(!result) {
		variant node = node_;

		boost::intrusive_ptr<game_logic::map_formula_callable> callable(new game_logic::map_formula_callable);
		callable->add("doc", variant(variant_callable::create(&node)));

		foreach(const std::string& v, variations) {
			std::map<std::string, game_logic::const_formula_ptr>::const_iterator var_itor = variations_.find(v);
			ASSERT_LOG(var_itor != variations_.end(), "COULD NOT FIND VARIATION " << v << " IN " << id_);

			variant cmd = var_itor->second->execute(*callable);

			execute_variation_command(cmd, *callable);

			//std::cerr << "VARIATION " << v << ":\n--- BEFORE ---\n" << node.write_json() << "\n--- AFTER ---\n" << node_.write_json() << "\n--- DONE ---\n";
		}

		//set our constants so the variation can decide whether it needs
		//to re-parse formulas or not.
		const game_logic::constants_loader scope_consts(node_["consts"]);

		//copy the id over from the parent object, to make sure it's
		//the same. This is important for nested objects.
		custom_object_type* obj_type = new custom_object_type(node, this);
		obj_type->id_ = id_;
		result.reset(obj_type);
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
		module::get_unique_filenames_under_dir("data/objects", &file_paths);
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

UTILITY(test_all_objects)
{
	custom_object_type::get_all();
}
