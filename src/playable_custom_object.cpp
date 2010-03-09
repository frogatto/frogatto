#include "joystick.hpp"
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

bool playable_custom_object::is_active(const rect& screen_area) const
{
	//player objects are always active.
	return true;
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

namespace {
	static const std::string ctrl[] = { "ctrl_up", "ctrl_down", "ctrl_left", "ctrl_right", "ctrl_attack", "ctrl_jump" };
}

variant playable_custom_object::get_value(const std::string& key) const
{
	if(key == "ctrl_tilt") {
		return variant(-joystick::iphone_tilt());
	}

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
	} else if(key == "control_lock") {
		if(value.is_null() && control_lock_.get() != NULL) {
			control_lock_.reset();
		} else if(value.is_list()) {
			unsigned char state = 0;
			for(int n = 0; n != value.num_elements(); ++n) {
				ASSERT_LOG(value[n].is_string(), "MEMBER OF control_lock LIST NOT A STRING");
				const std::string& str = value[n].as_string();
				int control_key = -1;
				for(int m = 0; m != sizeof(ctrl)/sizeof(*ctrl); ++m) {
					if(ctrl[m] == str) {
						control_key = m;
						break;
					}
				}

				ASSERT_LOG(control_key != -1, "ILLEGAL STRING SET FOR control_lock: '" << str << "' LEGAL KEYS ARE ctrl_(up|down|left|right|attack|jump)");
				state |= 1 << control_key;
			}

			std::cerr << "SET CONTROL STATE: " << (int)state << "\n";

			//destroy the old one before creating a new control_lock,
			//since control_lock objects must be constructed and destroyed
			//in FIFO order.
			control_lock_.reset();
			control_lock_.reset(new controls::local_controls_lock(state));
		} else {
			ASSERT_LOG(false, "BAD VALUE WHEN SETTING control_lock KEY. A LIST OR NULL IS REQUIRED.");
		}
	} else {
		custom_object::set_value(key, value);
	}
}
