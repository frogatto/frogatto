#include "asserts.hpp"
#include "custom_object_callable.hpp"

namespace {
std::string CustomObjectProperties[] = {
	"consts", "type", "time_in_animation", "time_in_animation_delta", "level", "animation",
	"hitpoints", "max_hitpoints", "mass", "label", "x", "y", "z", "zorder",
	"x1", "x2", "y1", "y2", "w", "h", "midpoint_x", "midpoint_y",
	"solid_rect", "img_w", "img_h", "front", "back", "cycle", "facing",
	"upside_down", "up", "down", "velocity_x", "velocity_y",
	"accel_x", "accel_y", "vars", "tmp", "group", "rotate", "me", "self",
	"red", "green", "blue", "alpha", "damage", "hit_by", "jumped_on_by",
	"distortion", "is_standing", "near_cliff_edge", "distance_to_cliff",
	"slope_standing_on", "underwater", "water_bounds", "driver", "is_human", "invincible",
	"sound_volume", "destroyed", "is_standing_on_platform", "standing_on",
	"fragment_shaders", "vertex_shaders", "shader", "variations",
	"attached_objects", "call_stack",
	"solid_dimensions_in", "solid_dimensions_not_in", "brightness",
	"current_generator", "tags", "draw_area", "activation_area",
	"fall_through_platforms", "has_feet",
	"ctrl_up", "ctrl_down", "ctrl_left", "ctrl_right",
	"ctrl_attack", "ctrl_jump",
};

}

const custom_object_callable& custom_object_callable::instance()
{
	static const custom_object_callable obj;
	return obj;
}

custom_object_callable::custom_object_callable()
{
	ASSERT_EQ(NUM_CUSTOM_OBJECT_PROPERTIES, sizeof(CustomObjectProperties)/sizeof(*CustomObjectProperties));

	for(int n = 0; n != sizeof(CustomObjectProperties)/sizeof(*CustomObjectProperties); ++n) {
		entries_.push_back(entry(CustomObjectProperties[n]));
	}

	for(int n = 0; n != entries_.size(); ++n) {
		keys_to_slots_[entries_[n].id] = n;
	}
}

int custom_object_callable::get_slot(const std::string& key) const
{
	std::map<std::string, int>::const_iterator itor = keys_to_slots_.find(key);
	if(itor == keys_to_slots_.end()) {
		return -1;
	}

	return itor->second;
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
