#include <cassert>
#include <iostream>

#include "asserts.hpp"
#include "collision_utils.hpp"
#include "custom_object_callable.hpp"
#include "custom_object_functions.hpp"
#include "custom_object_type.hpp"
#include "filesystem.hpp"
#include "formula_constants.hpp"
#include "object_events.hpp"
#include "preferences.hpp"
#include "solid_map.hpp"
#include "string_utils.hpp"
#include "wml_modify.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_schema.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"
#include "unit_test.hpp"

namespace {
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

typedef std::map<std::string, const_custom_object_type_ptr> object_map;

object_map& cache() {
	static object_map instance;
	return instance;
}

namespace {
const std::string BaseStr = "%PROTO%";
}

void merge_into_prototype(wml::node_ptr prototype_node, wml::node_ptr node)
{
	for(std::map<std::string, wml::const_node_ptr>::const_iterator i = node->base_elements().begin(); i != node->base_elements().end(); ++i) {
		prototype_node->set_base_element(i->first, i->second);
	}

	//we have a prototype node and an object node. We want to merge
	//them together to give one object definition. We will save
	//the final definition in the prototype_node, overwriting its
	//values with changes from the object node.
	//begin by setting attributes in the prototype node from
	//the object node.
	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		std::string::const_iterator base_itor = std::search(i->second.str().begin(), i->second.str().end(), BaseStr.begin(), BaseStr.end());
		if(base_itor != i->second.str().end()) {
			std::string base_attr = prototype_node->attr(i->first);
			if(base_attr.empty()) {
				base_attr = "null()";
			}
			
			std::string value = std::string(i->second.str().begin(), base_itor) + base_attr + std::string(base_itor + BaseStr.size(), i->second.str().end());

			wml::value val(value, i->second.filename(), i->second.line());
			prototype_node->set_attr(i->first, val);
		} else {
			prototype_node->set_attr(i->first, i->second);
		}
	}

	//mapping of animation nodes is kinda complicated: in the
	//prototype there can be one specification of each animation.
	//in objects there can be multiple specifications. Each
	//animation in the object inherits from the specification in
	//the prototype.
	//
	//We are going to build a completely fresh/new set of animations
	//in a vector, and then wipe out all current animations and
	//replace with these from the vector.
	std::vector<wml::node_ptr> animation_nodes;

	//go over every animation in the object, and see if the animation
	//is also defined in the prototype.
	FOREACH_WML_CHILD(anim_node, node, "animation") {
		wml::node_ptr target_anim = wml::find_child_by_attribute(prototype_node, "animation", "id", anim_node->attr("id"));
		if(target_anim) {
			//the animation is in the prototype, so we merge the
			//object's definition of the animation with the
			//prototype's.
			wml::node_ptr new_node(wml::deep_copy(target_anim));
			wml::merge_over(anim_node, new_node);
			animation_nodes.push_back(new_node);
		} else {
			//the animation isn't in the prototype, so just add
			//what is given in the object.
			animation_nodes.push_back(wml::deep_copy(anim_node));
		}
	}

	//now go over the prototype node and add any animations that don't
	//appear in the child.
	FOREACH_WML_CHILD(anim_node, prototype_node, "animation") {
		if(wml::find_child_by_attribute(node, "animation", "id", anim_node->attr("id"))) {
			//it's already in the child, so don't add it.
			continue;
		}

		animation_nodes.push_back(wml::deep_copy(anim_node));
	}

	prototype_node->clear_children("animation");
	foreach(wml::node_ptr node, animation_nodes) {
		prototype_node->add_child(node);
	}

	//now go over every element and copy them in.
	for(wml::node::all_child_iterator i = node->begin_children();
	    i != node->end_children(); ++i) {
		const std::string& name = (*i)->name();
		if(name == "animation") {
			//we handled animations above, so ignore this here.
			continue;
		}

		if(name == "tmp" || name == "vars" || name == "consts" || name == "editor_info") {
			//we like to merge in vars nodes into one vars definition
			//if both the object and prototype have vars definitions.
			wml::node_ptr target = prototype_node->get_child(name);
			if(target) {
				wml::merge_over(*i, target);
				continue;
			}
		}

		prototype_node->add_child(*i);
	}
}

}

//function which finds if a node has a prototype, and if so, applies the
//prototype to the node.
wml::node_ptr custom_object_type::merge_prototype(wml::node_ptr node)
{
	if(!node->has_attr("prototype")) {
		return node;
	}

	std::vector<std::string> protos = util::split(node->attr("prototype"));

	foreach(const std::string& proto, protos) {
		//look up the object's prototype and merge it in
		std::map<std::string, std::string>::const_iterator path_itor = prototype_file_paths().find(proto + ".cfg");
		ASSERT_LOG(path_itor != prototype_file_paths().end(), "Could not find file for prototype '" << node->attr("prototype") << "'");

		wml::node_ptr prototype_node = wml::parse_wml_from_file(path_itor->second);
		prototype_node = merge_prototype(prototype_node);
		merge_into_prototype(prototype_node, node);
		node = prototype_node;
	}
	return node;
}

const std::string* custom_object_type::get_object_path(const std::string& id)
{
	if(object_file_paths().empty()) {
		//find out the paths to all our files
		sys::get_unique_filenames_under_dir(object_file_path(), &object_file_paths());
		sys::get_unique_filenames_under_dir("data/object_prototypes", &prototype_file_paths());
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

	return result;
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
		//find out the paths to all our files
		sys::get_unique_filenames_under_dir(object_file_path(), &object_file_paths());
		sys::get_unique_filenames_under_dir("data/object_prototypes", &prototype_file_paths());
	}

	//find the file for the object we are loading.
	std::map<std::string, std::string>::const_iterator path_itor = object_file_paths().find(id + ".cfg");
	ASSERT_LOG(path_itor != object_file_paths().end(), "Could not find file for object '" << id << "'");

	try {
		wml::node_ptr node = wml::parse_wml_from_file(path_itor->second);

		node = merge_prototype(node);

		ASSERT_LOG(node->attr("id").str() == id, "IN " << path_itor->second << " OBJECT ID DOES NOT MATCH FILENAME");

		static const wml::schema* schema = wml::schema::get("custom_object");
		if(schema) {
			schema->validate_node(node);
		}

		//create the object and add it to our cache.
		custom_object_type_ptr result(new custom_object_type(node));

		//load the object's variations here to avoid pausing the game
		//when an object starts its variation.
		result->load_variations();
		return result;
	} catch(wml::parse_error& e) {
		ASSERT_LOG(false, "Error parsing WML for custom object in " << path_itor->second << ": " << e.message);
	} catch(wml::schema_error& e) {
		ASSERT_LOG(false, "Error loading object '" << id << "': " << e.message);
	} catch(...) {
		ASSERT_LOG(false, "Unknown error loading custom object in " << path_itor->second);
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


void custom_object_type::init_event_handlers(wml::const_node_ptr node,
                                             event_handler_map& handlers,
											 game_logic::function_symbol_table* symbols,
											 const event_handler_map* base_handlers)
{
	if(symbols == NULL) {
		symbols = &get_custom_object_functions_symbol_table();
	}

	static custom_object_callable custom_object_definition;

	for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
		if(i->first.size() > 3 && std::equal(i->first.begin(), i->first.begin() + 3, "on_")) {
			const std::string event(i->first.begin() + 3, i->first.end());
			const int event_id = get_object_event_id(event);
			if(handlers.size() <= event_id) {
				handlers.resize(event_id+1);
			}

			if(base_handlers && base_handlers->size() > event_id && (*base_handlers)[event_id] && (*base_handlers)[event_id]->str() == i->second.str()) {
				handlers[event_id] = (*base_handlers)[event_id];
			} else {
				handlers[event_id] = game_logic::formula::create_optional_formula(i->second, symbols, &custom_object_definition);
			}
		}
	}
}


custom_object_type::custom_object_type(wml::const_node_ptr node, const custom_object_type* base_type)
  : id_(node->attr("id")),
	hitpoints_(wml::get_int(node, "hitpoints", 1)),
	timer_frequency_(wml::get_int(node, "timer_frequency", -1)),
	zorder_(wml::get_int(node, "zorder")),
	zsub_order_(wml::get_int(node, "zsub_order")),
	is_human_(wml::get_bool(node, "is_human", false)),
	goes_inactive_only_when_standing_(wml::get_bool(node, "goes_inactive_only_when_standing", false)),
	dies_on_inactive_(wml::get_bool(node, "dies_on_inactive", false)),
	always_active_(wml::get_bool(node, "always_active", false)),
    body_harmful_(wml::get_bool(node, "body_harmful", true)),
    body_passthrough_(wml::get_bool(node, "body_passthrough", false)),
    ignore_collide_(wml::get_bool(node, "ignore_collide", false)),
    object_level_collisions_(wml::get_bool(node, "object_level_collisions", false)),
	surface_friction_(wml::get_int(node, "surface_friction", 100)),
	surface_traction_(wml::get_int(node, "surface_traction", 100)),
	friction_(wml::get_int(node, "friction")),
	traction_(wml::get_int(node, "traction", 1000)),
	traction_in_air_(wml::get_int(node, "traction_in_air", 0)),
	traction_in_water_(wml::get_int(node, "traction_in_water", 0)),
	respawns_(wml::get_bool(node, "respawns", true)),
	affected_by_currents_(wml::get_bool(node, "affected_by_currents", false)),
	is_vehicle_(wml::get_bool(node, "vehicle", false)),	
	passenger_x_(wml::get_int(node, "passenger_x")),
	passenger_y_(wml::get_int(node, "passenger_y")),
	feet_width_(wml::get_int(node, "feet_width", 0)),
	use_image_for_collisions_(wml::get_bool(node, "use_image_for_collisions", false)),
	has_feet_(wml::get_bool(node, "has_feet", true) && use_image_for_collisions_ == false),
	adjust_feet_on_animation_change_(wml::get_bool(node, "adjust_feet_on_animation_change", true)),
	teleport_offset_x_(wml::get_int(node, "teleport_offset_x")),
	teleport_offset_y_(wml::get_int(node, "teleport_offset_y")),
	no_move_to_standing_(wml::get_bool(node, "no_move_to_standing")),
	reverse_global_vertical_zordering_(wml::get_bool(node, "reverse_global_vertical_zordering", false)),
	serializable_(wml::get_bool(node, "serializable", true)),
	solid_(solid_info::create(node)),
	platform_(solid_info::create_platform(node)),
	has_solid_(solid_ || use_image_for_collisions_),
	solid_dimensions_(has_solid_ || platform_ ? 0xFFFFFFFF : 0),
	collide_dimensions_(0xFFFFFFFF),
	weak_solid_dimensions_(has_solid_ || platform_ ? 0xFFFFFFFF : 0),
	weak_collide_dimensions_(0xFFFFFFFF),
	activation_border_(wml::get_int(node, "activation_border", 100))
{
	if(node->get_child("editor_info")) {
		editor_info_.reset(new editor_entity_info(node->get_child("editor_info")));
	}

	const bool is_variation = base_type != NULL;

	//make it so any formula has these constants defined.
	const game_logic::constants_loader scope_consts(node->get_child("consts"));

	//if some constants change from base to variation, then we have to
	//re-parse all formulas.
	if(scope_consts.same_as_base() == false) {
		base_type = NULL;
	}

	if(node->has_attr("solid_dimensions")) {
		weak_solid_dimensions_ = solid_dimensions_ = 0;
		foreach(std::string key, util::split(node->attr("solid_dimensions"))) {
			if(key != "level_only") {
				if(key.empty() == false && key[0] == '~') {
					key = std::string(key.begin()+1, key.end());
					weak_solid_dimensions_ |= (1 << get_solid_dimension_id(key));
				} else {
					solid_dimensions_ |= (1 << get_solid_dimension_id(key));
				}
			}
		}

		weak_solid_dimensions_ |= solid_dimensions_;
	}

	if(node->has_attr("collide_dimensions")) {
		weak_collide_dimensions_ = collide_dimensions_ = 0;
		foreach(std::string key, util::split(node->attr("collide_dimensions"))) {
			if(key != "level_only") {
				if(key.empty() == false && key[0] == '~') {
					key = std::string(key.begin()+1, key.end());
					weak_collide_dimensions_ |= (1 << get_solid_dimension_id(key));
				} else {
					collide_dimensions_ |= (1 << get_solid_dimension_id(key));
				}
			}
		}

		weak_collide_dimensions_ |= collide_dimensions_;
	}

	wml::node::const_child_iterator a1 = node->begin_child("animation");
	wml::node::const_child_iterator a2 = node->end_child("animation");
	for(; a1 != a2; ++a1) {
		boost::shared_ptr<frame> f;
		try {
			f.reset(new frame(a1->second));
		} catch(frame::error&) {
			ASSERT_LOG(false, "ERROR LOADING FRAME IN OBJECT '" << id_ << "'");
		}

		if(use_image_for_collisions_) {
			f->set_image_as_solid();
		}

		if(f->solid()) {
			has_solid_ = true;
		}

		frames_[a1->second->attr("id")].push_back(f);
		const int duplicates = atoi(a1->second->attr("duplicates").c_str());
		if(duplicates > 1) {
			for(int n = 1; n != duplicates; ++n) {
				frames_[a1->second->attr("id")].push_back(f);
			}
		}
		if(!default_frame_) {
			default_frame_ = f;
		}
	}

	std::vector<variant> available_frames;
	for(frame_map::const_iterator i = frames_.begin(); i != frames_.end(); ++i) {
		available_frames.push_back(variant(i->first));
	}

	available_frames_ = variant(&available_frames);

	mass_ = wml::get_int(node, "mass", (default_frame_->collide_w() * default_frame_->collide_h() ) );
	
	wml::node::const_child_iterator c1 = node->begin_child("child");
	wml::node::const_child_iterator c2 = node->end_child("child");
	for(; c1 != c2; ++c1) {
		const std::string& child_id = c1->second->attr("child_id");
		children_[child_id] = c1->second;
	}

	assert(default_frame_);

	next_animation_formula_ = game_logic::formula::create_optional_formula(node->attr("next_animation"), function_symbols());

	FOREACH_WML_CHILD(particle_node, node, "particle_system") {
		particle_factories_[particle_node->attr("id")] = particle_system_factory::create_factory(particle_node);
	}

	if(!is_variation) {
		FOREACH_WML_CHILD(object_node, node, "object_type") {
			wml::node_ptr dup_object_node = wml::deep_copy(object_node);
			custom_object_type* type = new custom_object_type(merge_prototype(dup_object_node));
			type->id_ = id_ + "." + type->id_;
			sub_objects_[object_node->attr("id")].reset(type);
		}
	}

	if(node->has_attr("parallax_scale_x") || node->has_attr("parallax_scale_y")) {
		parallax_scale_millis_.reset(new std::pair<int, int>(wml::get_int(node, "parallax_scale_x", 1000), wml::get_int(node, "parallax_scale_y", 1000)));
	}
	
	wml::const_node_ptr vars = node->get_child("vars");
	if(vars) {
		std::vector<std::string> var_str;
		for(wml::node::const_attr_iterator v = vars->begin_attr(); v != vars->end_attr(); ++v) {
			variant var;
			var.serialize_from_string(v->second);
			variables_[v->first] = var;
			var_str.push_back(v->first);
		}

		game_logic::formula_callable_definition::entry* entry = callable_definition_.get_entry(CUSTOM_OBJECT_VARS);
		ASSERT_LOG(entry != NULL, "CANNOT FIND VARS ENTRY IN OBJECT");
		entry->type_definition_holder = game_logic::create_formula_callable_definition(&var_str[0], &var_str[0] + var_str.size());
		entry->type_definition = entry->type_definition_holder.get();
	}

	wml::const_node_ptr tmp_vars = node->get_child("tmp");
	if(tmp_vars) {
		std::vector<std::string> var_str;
		for(wml::node::const_attr_iterator v = tmp_vars->begin_attr(); v != tmp_vars->end_attr(); ++v) {
			variant var;
			var.serialize_from_string(v->second);
			tmp_variables_[v->first] = var;
			var_str.push_back(v->first);
		}

		game_logic::formula_callable_definition::entry* entry = callable_definition_.get_entry(CUSTOM_OBJECT_TMP);
		ASSERT_LOG(entry != NULL, "CANNOT FIND TMP ENTRY IN OBJECT");
		entry->type_definition_holder = game_logic::create_formula_callable_definition(&var_str[0], &var_str[0] + var_str.size());
		entry->type_definition = entry->type_definition_holder.get();
	}

	//std::cerr << "TMP_VARIABLES: '" << id_ << "' -> " << tmp_variables_.size() << "\n";

	consts_.reset(new game_logic::map_formula_callable);
	wml::const_node_ptr consts = node->get_child("consts");
	if(consts) {
		for(wml::node::const_attr_iterator v = consts->begin_attr(); v != consts->end_attr(); ++v) {
			variant var;
			var.serialize_from_string(v->second);
			consts_->add(v->first, var);
		}
	}

	if(node->has_attr("tags")) {
		const std::vector<std::string> tags = util::split(node->attr("tags"));
		foreach(const std::string& tag, tags) {
			tags_[tag] = variant(1);
		}
	}

	FOREACH_WML_CHILD(properties_node, node, "properties") {
		for(wml::node::const_attr_iterator i = properties_node->begin_attr(); i != properties_node->end_attr(); ++i) {
			properties_[i->first] = game_logic::formula::create_optional_formula(i->second, function_symbols(), &callable_definition_);
		}
	}

	FOREACH_WML_CHILD(variation_node, node, "object_variation") {
		const std::string& id = variation_node->attr("id");
		variations_[id].reset(new wml::modifier(variation_node));
		node_ = node;
	}

	game_logic::register_formula_callable_definition("object_type", &callable_definition_);

	if(base_type) {
		//if we're a variation, just get the functions from our base type.
		//variations can't define new functions.
		object_functions_ = base_type->object_functions_;
	} else if(node->has_attr("functions")) {
		object_functions_.reset(new game_logic::function_symbol_table);
		object_functions_->set_backup(&get_custom_object_functions_symbol_table());
		game_logic::formula f(node->attr("functions"), object_functions_.get());
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
	ASSERT_LOG(i != particle_factories_.end(), "Unknown particle system type in " << id_ << ": " << id)
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

const_custom_object_type_ptr custom_object_type::get_variation(const std::vector<std::string>& variations) const
{
	ASSERT_LOG(node_, "tried to set variation in object which has no variations");

	const_custom_object_type_ptr& result = variations_cache_[variations];
	if(!result) {
		wml::node_ptr node = wml::deep_copy(node_);
		foreach(const std::string& v, variations) {
			std::map<std::string, wml::const_modifier_ptr>::const_iterator var_itor = variations_.find(v);
			if(var_itor != variations_.end() && var_itor->second) {
				var_itor->second->modify(node);
			}
		}

		//set our constants so the variation can decide whether it needs
		//to re-parse formulas or not.
		const game_logic::constants_loader scope_consts(node_->get_child("consts"));

		result.reset(new custom_object_type(node, this));
	}

	return result;
}

void custom_object_type::load_variations() const
{
	if(!node_ || variations_.empty() || !node_->has_attr("load_variations")) {
		return;
	}

	const std::vector<std::string> variations_to_load = util::split(node_->attr("load_variations"));
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

		wml::node_ptr obj_node = wml::parse_wml_from_file(*fname);
		wml::node_ptr node = custom_object_type::merge_prototype(obj_node);

		std::cout << "OBJECT " << arg << "\n---\n" << wml::output(node) << "\n---\n";
	}
}
