#include "collision_utils.hpp"
#include "formatter.hpp"
#include "preferences.hpp"
#include "iphone_controls.hpp"
#include "joystick.hpp"
#include "level.hpp"
#include "playable_custom_object.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

playable_custom_object::playable_custom_object(const custom_object& obj)
  : custom_object(obj), player_info_(*this), difficulty_(0), vertical_look_(0),
    underwater_ctrl_x_(0), underwater_ctrl_y_(0), underwater_controls_(false),
	can_interact_(0)
{
}

playable_custom_object::playable_custom_object(const playable_custom_object& obj)
  : custom_object(obj), player_info_(obj.player_info_), difficulty_(0),
    save_condition_(obj.save_condition_), vertical_look_(0),
    underwater_ctrl_x_(0), underwater_ctrl_y_(0), underwater_controls_(false),
	can_interact_(0)
{
	player_info_.set_entity(*this);
}

playable_custom_object::playable_custom_object(variant node)
  : custom_object(node), player_info_(*this, node),
    difficulty_(node["difficulty"].as_int(0)),
    vertical_look_(0), underwater_ctrl_x_(0), underwater_ctrl_y_(0),
	underwater_controls_(node["underwater_controls"].as_bool(false)),
	can_interact_(0)
{
}

variant playable_custom_object::write() const
{
	variant_builder node;
	node.merge_object(custom_object::write());
	node.merge_object(player_info_.write());
	node.add("is_human", true);
	if(difficulty_) {
		node.add("difficulty", difficulty_);
	}

	if(underwater_controls_) {
		node.add("underwater_controls", true);
	}
	return node.build();
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

bool playable_custom_object::on_platform() const
{
	collision_info stand_info;
	const bool standing = is_standing(level::current(), &stand_info);
	return standing && stand_info.platform;
}

int playable_custom_object::walk_up_or_down_stairs() const
{
	return control_status(controls::CONTROL_DOWN) - control_status(controls::CONTROL_UP);
}

void playable_custom_object::process(level& lvl)
{
	if(player_info_.current_level() != lvl.id()) {
		player_info_.set_current_level(lvl.id());
	}

	if(can_interact_ > 0) {
		--can_interact_;
	}

	iphone_controls::set_underwater(underwater_controls_);
	iphone_controls::set_can_interact(can_interact_ != 0);
	iphone_controls::set_on_platform(on_platform());
	iphone_controls::set_standing(is_standing(level::current()));

	float underwater_x, underwater_y;
	if(underwater_controls_ && iphone_controls::water_dir(&underwater_x, &underwater_y)) {
		underwater_ctrl_x_ = underwater_x*1000;
		underwater_ctrl_y_ = underwater_y*1000;
	} else {
		underwater_ctrl_x_ = 0;
		underwater_ctrl_y_ = 0;
	}
	
	reverse_ab_ = preferences::reverse_ab();

	bool controls[controls::NUM_CONTROLS];
	for(int n = 0; n != controls::NUM_CONTROLS; ++n) {
		controls[n] = control_status(static_cast<controls::CONTROL_ITEM>(n));
	}

	clear_control_status();
	read_controls(lvl.cycle());
	static const std::string keys[] = { "up", "down", "left", "right", "attack", "jump", "tongue" };	
	for(int n = 0; n != controls::NUM_CONTROLS; ++n) {
		if(controls[n] != control_status(static_cast<controls::CONTROL_ITEM>(n))) {
			if(controls[n]) {
				handle_event("end_ctrl_" + keys[n]);
			} else {
				handle_event("ctrl_" + keys[n]);
			}
		}
	}

	custom_object::process(lvl);

}

namespace {
	static const char* ctrl[] = { "ctrl_up", "ctrl_down", "ctrl_left", "ctrl_right", "ctrl_attack", "ctrl_jump", "ctrl_tongue" };
}

variant playable_custom_object::get_value(const std::string& key) const
{
	if(key == "difficulty") {
		if(preferences::force_difficulty() != INT_MIN) {
			return variant(preferences::force_difficulty());
		}

		return variant(difficulty_);
	} else if(key == "can_interact") {
		return variant(can_interact_);
	} else if(key == "underwater_controls") {
		return variant(underwater_controls_);
	} else if(key == "ctrl_mod_key") {
		return variant(SDL_GetModState());
	} else if(key == "ctrl_keys") {
		std::vector<variant> result;
		#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		#else
#if defined(__ANDROID__) && SDL_VERSION_ATLEAST(1, 3, 0)
		Uint8* key_state = SDL_GetKeyboardState(NULL);
		for(int count = 0; count < SDL_NUM_SCANCODES; count++) {
#else
		Uint8* key_state = SDL_GetKeyState(NULL);
		for(int count = SDLK_FIRST; count < SDLK_LAST; count++) {
#endif
			if(key_state[count]) {				//Returns only keys that are down so the list that ffl has to deal with is small.
				if(util::c_isprint(count)) {
					std::string str(1,count);
					result.push_back(variant(str));
				} else {
					result.push_back(variant(count));
				}
			}
		}
		#endif
		return variant(&result);
	} else if(key == "ctrl_mice") {
		std::vector<variant> result;
		

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		const int nmice = SDL_GetNumMice();
#else
		const int nmice = 1;
#endif
		for(int n = 0; n != nmice; ++n) {
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
			SDL_SelectMouse(n);
#endif
			std::vector<variant> info;
			int x, y;
			Uint8 button_state = SDL_GetMouseState(&x, &y);
			translate_mouse_coords(&x, &y);

			info.push_back(variant(x));
			info.push_back(variant(y));

			if(button_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				info.push_back(variant("left"));
			}

			if(button_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
				info.push_back(variant("right"));
			}

			if(button_state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
				info.push_back(variant("middle"));
			}

			if(button_state & SDL_BUTTON(SDL_BUTTON_X1)) { //these aren't tested
				info.push_back(variant("x1"));
			}

			if(button_state & SDL_BUTTON(SDL_BUTTON_X2)) {
				info.push_back(variant("x2"));
			}

			if(button_state & SDL_BUTTON(SDL_BUTTON_WHEELUP)) { //and these don't work
				info.push_back(variant("up"));
			}

			if(button_state & SDL_BUTTON(SDL_BUTTON_WHEELDOWN)) {
				info.push_back(variant("down"));
			}
			
			result.push_back(variant(&info));
		}

		return variant(&result);
	} else if(key == "ctrl_tilt") {
		return variant(-joystick::iphone_tilt());
	} else if(key == "ctrl_x") {
		return variant(underwater_ctrl_x_);
	} else if(key == "ctrl_y") {
		return variant(underwater_ctrl_y_);
	} else if(key == "ctrl_reverse_ab") {
		return variant(reverse_ab_);
	} else if(key == "control_scheme") {
		return variant(preferences::control_scheme());
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
	if(key == "difficulty") {
		difficulty_ = value.as_int();
	} else if(key == "can_interact") {
		can_interact_ = value.as_int();
	} else if(key == "underwater_controls") {
		underwater_controls_ = value.as_bool();
	} else if(key == "vertical_look") {
		vertical_look_ = value.as_int();
	} else if(key == "control_lock") {
		if(value.is_null()) {
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

			//destroy the old one before creating a new control_lock,
			//since control_lock objects must be constructed and destroyed
			//in FIFO order.
			control_lock_.reset();
			control_lock_.reset(new controls::local_controls_lock(state));
		} else {
			ASSERT_LOG(false, "BAD VALUE WHEN SETTING control_lock KEY. A LIST OR NULL IS REQUIRED: " << value.to_debug_string());
		}
	} else {
		custom_object::set_value(key, value);
	}
}
