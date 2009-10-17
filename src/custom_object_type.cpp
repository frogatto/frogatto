#include <cassert>
#include <iostream>

#include "asserts.hpp"
#include "custom_object_functions.hpp"
#include "custom_object_type.hpp"
#include "filesystem.hpp"
#include "solid_map.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"

namespace {
std::map<std::string, std::string> object_file_paths, prototype_file_paths;

typedef std::map<std::string, const_custom_object_type_ptr> object_map;
object_map cache;
}

const_custom_object_type_ptr custom_object_type::get(const std::string& id)
{
	object_map::const_iterator itor = cache.find(id);
	if(itor != cache.end()) {
		return itor->second;
	}

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

		if(node->has_attr("prototype")) {
			//look up the object's prototype and merge it in
			std::map<std::string, std::string>::const_iterator path_itor = prototype_file_paths.find(node->attr("prototype").str() + ".cfg");
			ASSERT_LOG(path_itor != prototype_file_paths.end(), "Could not find file for prototype '" << node->attr("prototype") << "'");

			wml::node_ptr prototype_node = wml::parse_wml_from_file(path_itor->second);

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

				if(name == "vars") {
					//we like to merge in vars nodes into one vars definition
					//if both the object and prototype have vars definitions.
					wml::node_ptr target = prototype_node->get_child("vars");
					if(target) {
						wml::merge_over(*i, target);
						continue;
					}
				}

				prototype_node->add_child(*i);
			}
			    
			node = prototype_node;
		}

		//create the object and add it to our cache.
		custom_object_type_ptr result(new custom_object_type(node));
		cache[id] = result;
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
    springiness_(wml::get_int(node, "springiness")),
	surface_friction_(wml::get_int(node, "surface_friction", 20)),
	surface_traction_(wml::get_int(node, "surface_traction", 100)),
	friction_(wml::get_int(node, "friction")),
	traction_(wml::get_int(node, "traction", 1000)),
	traction_in_air_(wml::get_int(node, "traction_in_air", 0)),
	on_players_side_(wml::get_bool(node, "on_players_side", false)),
	respawns_(wml::get_bool(node, "respawns", true)),
	affected_by_currents_(wml::get_bool(node, "affected_by_currents", false)),
	is_vehicle_(wml::get_bool(node, "vehicle", false)),	
	passenger_x_(wml::get_int(node, "passenger_x")),
	passenger_y_(wml::get_int(node, "passenger_y")),
	feet_width_(wml::get_int(node, "feet_width", 5)),
	use_image_for_collisions_(wml::get_bool(node, "use_image_for_collisions", false)),
	teleport_offset_x_(wml::get_int(node, "teleport_offset_x")),
	teleport_offset_y_(wml::get_int(node, "teleport_offset_y")),
	solid_(solid_info::create(node)),
	platform_(solid_info::create_platform(node))
{
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

	if(node->has_attr("tags")) {
		tags_ = util::split(node->attr("tags"));
	}
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
