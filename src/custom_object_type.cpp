#include <cassert>
#include <iostream>

#include "asserts.hpp"
#include "collision_utils.hpp"
#include "custom_object_functions.hpp"
#include "custom_object_type.hpp"
#include "filesystem.hpp"
#include "solid_map.hpp"
#include "string_utils.hpp"
#include "wml_modify.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"
#include "unit_test.hpp"

namespace {
std::map<std::string, std::string> object_file_paths, prototype_file_paths;

typedef std::map<std::string, const_custom_object_type_ptr> object_map;
object_map cache;

void merge_into_prototype(wml::node_ptr prototype_node, wml::node_ptr node)
{
	//we have a prototype node and an object node. We want to merge
	//them together to give one object definition. We will save
	//the final definition in the prototype_node, overwriting its
	//values with changes from the object node.
	//begin by setting attributes in the prototype node from
	//the object node.
	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		prototype_node->set_attr(i->first, i->second);
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

		if(name == "vars" || name == "consts") {
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

//function which finds if a node has a prototype, and if so, applies the
//prototype to the node.
wml::node_ptr merge_prototype(wml::node_ptr node)
{
	if(!node->has_attr("prototype")) {
		return node;
	}

	std::vector<std::string> protos = util::split(node->attr("prototype"));

	foreach(const std::string& proto, protos) {
		//look up the object's prototype and merge it in
		std::map<std::string, std::string>::const_iterator path_itor = prototype_file_paths.find(proto + ".cfg");
		ASSERT_LOG(path_itor != prototype_file_paths.end(), "Could not find file for prototype '" << node->attr("prototype") << "'");

		wml::node_ptr prototype_node = wml::parse_wml_from_file(path_itor->second);
		prototype_node = merge_prototype(prototype_node);
		merge_into_prototype(prototype_node, node);
		node = prototype_node;
	}
	return node;
}

}

const_custom_object_type_ptr custom_object_type::get(const std::string& id)
{
	object_map::const_iterator itor = cache.find(id);
	if(itor != cache.end()) {
		return itor->second;
	}

	const_custom_object_type_ptr result(create(id));
	cache[id] = result;
	return result;
}

custom_object_type_ptr custom_object_type::create(const std::string& id)
{
	if(object_file_paths.empty()) {
		//find out the paths to all our files
		sys::get_unique_filenames_under_dir("data/objects", &object_file_paths);
		sys::get_unique_filenames_under_dir("data/object_prototypes", &prototype_file_paths);
	}

	//find the file for the object we are loading.
	std::map<std::string, std::string>::const_iterator path_itor = object_file_paths.find(id + ".cfg");
	ASSERT_LOG(path_itor != object_file_paths.end(), "Could not find file for object '" << id << "'");

	try {
		wml::node_ptr node = wml::parse_wml_from_file(path_itor->second);

		node = merge_prototype(node);

		std::cerr << "NODE FOR " << id << ": {{{\n";
		std::cerr << wml::output(node) << "\n}}}\n";

		//create the object and add it to our cache.
		custom_object_type_ptr result(new custom_object_type(node));
		return result;
	} catch(wml::parse_error& e) {
		ASSERT_LOG(false, "Error parsing WML for custom object in " << path_itor->second << ": " << e.message);
	} catch(...) {
		ASSERT_LOG(false, "Unknown error loading custom object in " << path_itor->second);
	}
}

void custom_object_type::init_event_handlers(wml::const_node_ptr node,
                                             event_handler_map& handlers,
											 game_logic::function_symbol_table* symbols)
{
	if(symbols == NULL) {
		symbols = &get_custom_object_functions_symbol_table();
	}

	for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
		if(i->first.size() > 3 && std::equal(i->first.begin(), i->first.begin() + 3, "on_")) {
			const std::string event(i->first.begin() + 3, i->first.end());
			handlers[event] = game_logic::formula::create_optional_formula(i->second, symbols);
		}
	}
}


custom_object_type::custom_object_type(wml::const_node_ptr node)
  : id_(node->attr("id")),
	hitpoints_(wml::get_int(node, "hitpoints", 1)),
	timer_frequency_(wml::get_int(node, "timer_frequency", -1)),
	zorder_(wml::get_int(node, "zorder")),
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
	on_players_side_(wml::get_bool(node, "on_players_side", false)),
	respawns_(wml::get_bool(node, "respawns", true)),
	affected_by_currents_(wml::get_bool(node, "affected_by_currents", false)),
	is_vehicle_(wml::get_bool(node, "vehicle", false)),	
	passenger_x_(wml::get_int(node, "passenger_x")),
	passenger_y_(wml::get_int(node, "passenger_y")),
	feet_width_(wml::get_int(node, "feet_width", 5)),
	use_image_for_collisions_(wml::get_bool(node, "use_image_for_collisions", false)),
	has_feet_(wml::get_bool(node, "has_feet", true) && use_image_for_collisions_ == false),
	teleport_offset_x_(wml::get_int(node, "teleport_offset_x")),
	teleport_offset_y_(wml::get_int(node, "teleport_offset_y")),
	solid_(solid_info::create(node)),
	platform_(solid_info::create_platform(node)),
	has_solid_(solid_ || use_image_for_collisions_),
	solid_dimensions_(has_solid_ || platform_ ? 0xFFFFFFFF : 0)
{

	if(node->has_attr("solid_dimensions")) {
		solid_dimensions_ = 0;
		foreach(const std::string& key, util::split(node->attr("solid_dimensions"))) {
			if(key != "level_only") {
				solid_dimensions_ |= (1 << get_solid_dimension_id(key));
			}
		}

		std::cerr << "SOLID DIMENSIONS " << id_ << ": " << node->attr("solid_dimensions") << " -> " << solid_dimensions_ << "\n";
		
	}

	if(node->has_attr("functions")) {
		object_functions_.reset(new game_logic::function_symbol_table);
		object_functions_->set_backup(&get_custom_object_functions_symbol_table());
		game_logic::formula f(node->attr("functions"), object_functions_.get());
	}

	wml::node::const_child_iterator a1 = node->begin_child("animation");
	wml::node::const_child_iterator a2 = node->end_child("animation");
	for(; a1 != a2; ++a1) {
		boost::shared_ptr<frame> f(new frame(a1->second));
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

	mass_ = wml::get_int(node, "mass", (default_frame_->collide_w() * default_frame_->collide_h() ) );
	
	wml::node::const_child_iterator c1 = node->begin_child("child");
	wml::node::const_child_iterator c2 = node->end_child("child");
	for(; c1 != c2; ++c1) {
		const std::string& child_id = c1->second->attr("child_id");
		children_[child_id] = c1->second;
	}

	assert(default_frame_);

	next_animation_formula_ = game_logic::formula::create_optional_formula(node->attr("next_animation"), function_symbols());

	init_event_handlers(node, event_handlers_, function_symbols());

	FOREACH_WML_CHILD(particle_node, node, "particle_system") {
		particle_factories_[particle_node->attr("id")] = particle_system_factory::create_factory(particle_node);
	}

	wml::const_node_ptr vars = node->get_child("vars");
	if(vars) {
		for(wml::node::const_attr_iterator v = vars->begin_attr(); v != vars->end_attr(); ++v) {
			variant var;
			var.serialize_from_string(v->second);
			variables_[v->first] = var;
		}
	}

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
			properties_[i->first] = game_logic::formula::create_optional_formula(i->second, function_symbols());
		}
	}

	FOREACH_WML_CHILD(variation_node, node, "object_variation") {
		const std::string& id = variation_node->attr("id");
		variations_[id].reset(new wml::modifier(variation_node));
	}

	if(!variations_.empty()) {
		node_ = node;
	}
}

custom_object_type::~custom_object_type()
{
	std::cerr << "DESTROY TYPE: '" << id_ << "'\n";
}

const frame& custom_object_type::default_frame() const
{
	return *default_frame_;
}

const frame& custom_object_type::get_frame(const std::string& key) const
{
	frame_map::const_iterator itor = frames_.find(key);
	if(itor == frames_.end() || itor->second.empty()) {
		return default_frame();
	} else {
		if(itor->second.size() == 1) {
			return *itor->second.front().get();
		} else {
			return *itor->second[rand()%itor->second.size()].get();
		}
	}
}

game_logic::const_formula_ptr custom_object_type::get_event_handler(const std::string& event) const
{
	std::map<std::string, game_logic::const_formula_ptr>::const_iterator i = event_handlers_.find(event);
	if(i != event_handlers_.end()) {
		return i->second;
	} else {
		return game_logic::const_formula_ptr();
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

		result.reset(new custom_object_type(node));
	}

	return result;
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
