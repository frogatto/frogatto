#include "asserts.hpp"
#include "custom_object_callable.hpp"

#include <pthread.h>

namespace {
std::vector<custom_object_callable::entry>& global_entries() {
	static std::vector<custom_object_callable::entry> instance;
	return instance;
}

std::map<std::string, int>& keys_to_slots() {
	static std::map<std::string, int> instance;
	return instance;
}

}

const custom_object_callable& custom_object_callable::instance()
{
	static const custom_object_callable obj;
	return obj;
}

custom_object_callable::custom_object_callable()
{

	static const std::string CustomObjectProperties[] = {
	"consts", "type", "active",
	"time_in_animation", "time_in_animation_delta", "level",
	"animation", "available_animations",
	"hitpoints", "max_hitpoints", "mass", "label", "x", "y", "xy", "z",
	"relative_x", "relative_y", "spawned_by", "spawned_children",
	"parent", "pivot", "zorder", "zsub_order",
	"previous_y", "x1", "x2", "y1", "y2", "w", "h", "mid_x", "mid_y", "mid_xy", "midpoint_x", "midpoint_y", "midpoint_xy", 
	"solid_rect", "solid_mid_x", "solid_mid_y", "solid_mid_xy", 
	"img_mid_x", "img_mid_y", "img_mid_xy", "img_w", "img_h", "img_wh", "front", "back", "cycle", "facing",
	"upside_down", "up", "down", "velocity_x", "velocity_y", "velocity_xy", 
	"velocity_magnitude", "velocity_angle",
	"accel_x", "accel_y", "accel_xy", "gravity_shift", "platform_motion_x",
	"registry", "globals", "vars", "tmp", "group", "rotate",
	"me", "self",
	"red", "green", "blue", "alpha", "text_alpha", "damage", "hit_by",
	"distortion", "is_standing", "standing_info",
	"near_cliff_edge", "distance_to_cliff",
	"slope_standing_on", "underwater", "water_bounds", "water_object",
	"driver", "is_human", "invincible",
	"sound_volume", "destroyed", "is_standing_on_platform", "standing_on",
	"fragment_shaders", "vertex_shaders", "shader", "variations",
	"attached_objects", "call_stack", "lights",
	"solid_dimensions_in", "solid_dimensions_not_in",
	"collide_dimensions_in", "collide_dimensions_not_in",
	"brightness", "current_generator", "tags", "draw_area", "scale",
	"activation_area", "clip_area",
	"always_active", "activation_border", "fall_through_platforms", "has_feet",
	"x_schedule", "y_schedule", "rotation_schedule", "schedule_speed",
	"schedule_expires",
	"platform_area", "platform_offsets", "custom_draw", "event_handlers",
	"use_absolute_screen_coordinates",
	"widgets", "textv",
	"ctrl_up", "ctrl_down", "ctrl_left", "ctrl_right",
	"ctrl_attack", "ctrl_jump", "ctrl_tongue",
};
	ASSERT_EQ(NUM_CUSTOM_OBJECT_PROPERTIES, sizeof(CustomObjectProperties)/sizeof(*CustomObjectProperties));

	if(global_entries().empty()) {
		for(int n = 0; n != sizeof(CustomObjectProperties)/sizeof(*CustomObjectProperties); ++n) {
			global_entries().push_back(entry(CustomObjectProperties[n]));
		}

		for(int n = 0; n != global_entries().size(); ++n) {
			keys_to_slots()[global_entries()[n].id] = n;
		}
	}

	entries_ = global_entries();
}

int custom_object_callable::get_key_slot(const std::string& key)
{
	std::map<std::string, int>::const_iterator itor = keys_to_slots().find(key);
	if(itor == keys_to_slots().end()) {
		return -1;
	}

	return itor->second;
}

int custom_object_callable::get_slot(const std::string& key) const
{
	std::map<std::string, int>::const_iterator itor = properties_.find(key);
	if(itor == properties_.end()) {
		return get_key_slot(key);
	} else {
		return itor->second;
	}
}

game_logic::formula_callable_definition::entry* custom_object_callable::get_entry(int slot)
{
	if(slot < 0 || slot >= entries_.size()) {
		return NULL;
	}

	return &entries_[slot];
}

const game_logic::formula_callable_definition::entry* custom_object_callable::get_entry(int slot) const
{
	if(slot < 0 || slot >= entries_.size()) {
		return NULL;
	}

	return &entries_[slot];
}

void custom_object_callable::add_property(const std::string& id)
{
	properties_[id] = entries_.size();
	entries_.push_back(entry(id));
}
