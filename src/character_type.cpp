#include <iostream>

#include "character_type.hpp"
#include "custom_object_functions.hpp"
#include "formula.hpp"
#include "wml_modify.hpp"
#include "wml_node.hpp"
#include "wml_writer.hpp"
#include "wml_utils.hpp"

namespace {
typedef std::map<std::string, const_character_type_ptr> char_map;
char_map cache;
}

void character_type::init(wml::const_node_ptr node)
{
	wml::node::const_child_iterator c1 = node->begin_child("character");
	wml::node::const_child_iterator c2 = node->end_child("character");
	for(; c1 != c2; ++c1) {
		std::cerr << "load character type '" << c1->second->attr("id") << "'\n";
		cache[c1->second->attr("id")].reset(new character_type(c1->second));
	}
}

const_character_type_ptr character_type::get(const std::string& id)
{
	return cache[id];
}

namespace {
frame* create_frame(wml::const_node_ptr node, const std::string& node_name) {
	wml::const_node_ptr child = node->get_child(node_name);
	if(child && !wml::get_bool(child, "disabled")) {
		return new frame(child);
	}

	return NULL;
}
}

character_type::character_type(wml::const_node_ptr node)
  : wml_(node),
    id_(node->attr("id")),
    stand_(node->get_child("stand") ? node->get_child("stand") : node->get_child("walk")),
    walk_(wml::get_int(node, "walk")),
	jump_(wml::get_int(node, "jump")),
	boost_(wml::get_int(node, "boost")),
	glide_(wml::get_int(node, "glide")),
	climb_(wml::get_int(node, "climb")),
	hitpoints_(wml::get_int(node, "hitpoints")),
	springiness_(wml::get_int(node, "springiness")),
	friction_(wml::get_int(node, "friction", 10)),
	traction_(wml::get_int(node, "traction", 100)),
	weight_(wml::get_int(node, "weight", 1)),
	feet_width_(wml::get_int(node, "feet_width", 5)),
	is_vehicle_(wml::get_bool(node, "vehicle", false)),
	passenger_x_(wml::get_int(node, "passenger_x")),
	passenger_y_(wml::get_int(node, "passenger_y")),
	vehicle_die_object_(wml::get_str(node, "vehicle_die_object")),
	loop_sound_(wml::get_str(node, "loop_sound"))
{
	for(wml::node::const_attr_iterator attr = node->begin_attr(); attr != node->end_attr(); ++attr) {
		static const std::string on_start = "on_";
		const std::string& name = attr->first;
		if(name.size() > on_start.size() && std::equal(on_start.begin(), on_start.end(), name.begin())) {
			std::string frame_id = std::string(name.begin() + on_start.size(), name.end());
			event_handlers_[frame_id] = game_logic::formula::create_optional_formula(attr->second, &get_custom_object_functions_symbol_table());
			std::cerr << "PARSE ON_START FOR " << frame_id << "\n";
		}
	}

	stand_up_slope_frame_.reset(create_frame(node, "stand_up_slope"));
	stand_down_slope_frame_.reset(create_frame(node, "stand_down_slope"));
	portrait_frame_.reset(create_frame(node, "portrait"));
	name_frame_.reset(create_frame(node, "name"));
	icon_frame_.reset(create_frame(node, "icon"));
	idle_frame_.reset(create_frame(node, "idle"));
	turn_frame_.reset(create_frame(node, "turn"));
	walk_frame_.reset(create_frame(node, "walk"));
	run_frame_.reset(create_frame(node, "run"));
	jump_frame_.reset(create_frame(node, "jump"));
	fall_frame_.reset(create_frame(node, "fall"));
	crouch_frame_.reset(create_frame(node, "crouch"));
	roll_frame_.reset(create_frame(node, "roll"));
	lookup_frame_.reset(create_frame(node, "lookup"));
	gethit_frame_.reset(create_frame(node, "gethit"));
	attack_frame_.reset(create_frame(node, "attack"));
	jump_attack_frame_.reset(create_frame(node, "jump_attack"));
	fall_spin_attack_frame_.reset(create_frame(node, "fall_spin_attack"));
	up_attack_frame_.reset(create_frame(node, "up_attack"));
	run_attack_frame_.reset(create_frame(node, "run_attack"));
	die_frame_.reset(create_frame(node, "die"));
	fly_frame_.reset(create_frame(node, "fly"));
	slide_frame_.reset(create_frame(node, "slide"));
	spring_frame_.reset(create_frame(node, "spring"));
	push_frame_.reset(create_frame(node, "push"));
	swim_side_idle_frame_.reset(create_frame(node, "swim_side_idle"));
	swim_up_idle_frame_.reset(create_frame(node, "swim_up_idle"));
	swim_down_idle_frame_.reset(create_frame(node, "swim_down_idle"));
	swim_side_frame_.reset(create_frame(node, "swim_side"));
	swim_up_frame_.reset(create_frame(node, "swim_up"));
	swim_down_frame_.reset(create_frame(node, "swim_down"));


	//if we have one swim frame we must have them all
	if(!swim_up_frame_ || !swim_down_frame_ || !swim_side_frame_) {
		swim_up_frame_.reset();
		swim_down_frame_.reset();
		swim_side_frame_.reset();
	}
}

const_character_type_ptr character_type::get_modified(const wml::modifier& modifier) const
{
	wml::node_ptr node = wml::deep_copy(wml_);
//	std::cerr << "BEFORE: {{{" << wml::output(node) << "}}}\n";
	modifier.modify(node);
//	std::cerr << "AFTER: {{{" << wml::output(node) << "}}}\n";
	return const_character_type_ptr(new character_type(node));
}

game_logic::const_formula_ptr character_type::get_event_handler(const std::string& event_id) const
{
	std::map<std::string, game_logic::const_formula_ptr>::const_iterator itor = event_handlers_.find(event_id);
	if(itor != event_handlers_.end()) {
		return itor->second;
	}

	return game_logic::const_formula_ptr();
}
