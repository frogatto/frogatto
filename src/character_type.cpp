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
	loop_sound_(wml::get_str(node, "loop_sound")),
	on_attack_formula_(game_logic::formula::create_optional_formula(wml::get_str(node, "on_attack"), &get_custom_object_functions_symbol_table())),
	on_process_formula_(game_logic::formula::create_optional_formula(wml::get_str(node, "on_process"), &get_custom_object_functions_symbol_table())),
	on_die_formula_(game_logic::formula::create_optional_formula(wml::get_str(node, "on_die"), &get_custom_object_functions_symbol_table()))
{
	if(node->get_child("stand_up_slope")) {
		stand_up_slope_frame_.reset(new frame(node->get_child("stand_up_slope")));
	}

	if(node->get_child("stand_down_slope")) {
		stand_down_slope_frame_.reset(new frame(node->get_child("stand_down_slope")));
	}

	if(node->get_child("portrait")) {
		portrait_frame_.reset(new frame(node->get_child("portrait")));
	}

	if(node->get_child("name")) {
		name_frame_.reset(new frame(node->get_child("name")));
	}

	if(node->get_child("icon")) {
		icon_frame_.reset(new frame(node->get_child("icon")));
	}

	if(node->get_child("idle")) {
		idle_frame_.reset(new frame(node->get_child("idle")));
	}

	if(node->get_child("turn")) {
		turn_frame_.reset(new frame(node->get_child("turn")));
	}

	if(node->get_child("walk")) {
		walk_frame_.reset(new frame(node->get_child("walk")));
	}

	if(node->get_child("run")) {
		run_frame_.reset(new frame(node->get_child("run")));
	}

	if(node->get_child("jump")) {
		jump_frame_.reset(new frame(node->get_child("jump")));
	}

	if(node->get_child("fall")) {
		fall_frame_.reset(new frame(node->get_child("fall")));
	}

	if(node->get_child("crouch")) {
		crouch_frame_.reset(new frame(node->get_child("crouch")));
	}

	if(node->get_child("roll")) {
		roll_frame_.reset(new frame(node->get_child("roll")));
	}

	if(node->get_child("lookup")) {
		lookup_frame_.reset(new frame(node->get_child("lookup")));
	}

	if(node->get_child("gethit")) {
		gethit_frame_.reset(new frame(node->get_child("gethit")));
	}

	if(node->get_child("attack")) {
		attack_frame_.reset(new frame(node->get_child("attack")));
	}

	if(node->get_child("jump_attack")) {
		jump_attack_frame_.reset(new frame(node->get_child("jump_attack")));
	}

	if(node->get_child("up_attack")) {
		up_attack_frame_.reset(new frame(node->get_child("up_attack")));
	}

	if(node->get_child("run_attack")) {
		run_attack_frame_.reset(new frame(node->get_child("run_attack")));
	}

	if(node->get_child("die")) {
		die_frame_.reset(new frame(node->get_child("die")));
	}

	if(node->get_child("fly")) {
		fly_frame_.reset(new frame(node->get_child("fly")));
	}

	if(node->get_child("slide")) {
		slide_frame_.reset(new frame(node->get_child("slide")));
	}

	if(node->get_child("spring")) {
		spring_frame_.reset(new frame(node->get_child("spring")));
	}
}

const_character_type_ptr character_type::get_modified(const wml::modifier& modifier) const
{
	wml::node_ptr node = wml::deep_copy(wml_);
	std::cerr << "BEFORE: {{{" << wml::output(node) << "}}}\n";
	modifier.modify(node);
	std::cerr << "AFTER: {{{" << wml::output(node) << "}}}\n";
	return const_character_type_ptr(new character_type(node));
}
