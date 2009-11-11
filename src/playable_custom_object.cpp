#include "level.hpp"
#include "playable_custom_object.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

playable_custom_object::playable_custom_object(const custom_object& obj)
  : custom_object(obj), player_info_(*this), vertical_look_(0)
{
}

playable_custom_object::playable_custom_object(const playable_custom_object& obj)
  : custom_object(obj), player_info_(obj.player_info_), save_condition_(obj.save_condition_), vertical_look_(0)
{
	player_info_.set_entity(*this);
}

playable_custom_object::playable_custom_object(wml::const_node_ptr node)
  : custom_object(node), player_info_(*this), vertical_look_(0)
{
}

wml::node_ptr playable_custom_object::write() const
{
	wml::node_ptr node = custom_object::write();
	node->set_attr("is_human", "true");
	return node;
}

void playable_custom_object::save_game()
{
	save_condition_ = clone();
}

entity_ptr playable_custom_object::backup() const
{
	return entity_ptr(new playable_custom_object(*this));
}

entity_ptr playable_custom_object::clone() const
{
	return entity_ptr(new playable_custom_object(*this));
}

void playable_custom_object::process(level& lvl)
{
	if(player_info_.current_level() != lvl.id()) {
		player_info_.set_current_level(lvl.id());
	}

	custom_object::process(lvl);

	bool controls[controls::NUM_CONTROLS];
	for(int n = 0; n != controls::NUM_CONTROLS; ++n) {
		controls[n] = control_status(static_cast<controls::CONTROL_ITEM>(n));
	}

	clear_control_status();
	read_controls(lvl.cycle());
	static const std::string keys[] = { "up", "down", "left", "right", "attack", "jump" };	
	for(int n = 0; n != controls::NUM_CONTROLS; ++n) {
		if(controls[n] != control_status(static_cast<controls::CONTROL_ITEM>(n))) {
			if(controls[n]) {
				handle_event("end_ctrl_" + keys[n]);
			} else {
				handle_event("ctrl_" + keys[n]);
			}
		}
	}
}

variant playable_custom_object::get_value(const std::string& key) const
{
	static const std::string ctrl[] = { "ctrl_up", "ctrl_down", "ctrl_left", "ctrl_right", "ctrl_attack", "ctrl_jump" };
	for(int n = 0; n < sizeof(ctrl)/sizeof(*ctrl); ++n) {
		if(key == ctrl[n]) {
			return variant(control_status(static_cast<controls::CONTROL_ITEM>(n)));
		}
	}

	if(key == "player") {
		return variant(1);
	} else if(key == "vertical_look") {
		return variant(vertical_look_);
	}

	return custom_object::get_value(key);
}

void playable_custom_object::set_value(const std::string& key, const variant& value)
{
	if(key == "vertical_look") {
		vertical_look_ = value.as_int();
	} else {
		custom_object::set_value(key, value);
	}
}
