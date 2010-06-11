#include "asserts.hpp"
#include "custom_object_callable.hpp"

namespace {
std::vector<custom_object_callable::entry>& entries() {
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
	"time_in_animation", "time_in_animation_delta", "level", "animation",
	"hitpoints", "max_hitpoints", "mass", "label", "x", "y", "z", "zorder",
	"previous_y", "x1", "x2", "y1", "y2", "w", "h", "midpoint_x", "midpoint_y",
	"solid_rect", "img_w", "img_h", "front", "back", "cycle", "facing",
	"upside_down", "up", "down", "velocity_x", "velocity_y",
	"accel_x", "accel_y", "vars", "tmp", "group", "rotate", "me", "self",
	"red", "green", "blue", "alpha", "text_alpha", "damage", "hit_by",
	"distortion", "is_standing", "near_cliff_edge", "distance_to_cliff",
	"slope_standing_on", "underwater", "water_bounds", "water_object",
	"driver", "is_human", "invincible",
	"sound_volume", "destroyed", "is_standing_on_platform", "standing_on",
	"fragment_shaders", "vertex_shaders", "shader", "variations",
	"attached_objects", "call_stack",
	"solid_dimensions_in", "solid_dimensions_not_in",
	"collide_dimensions_in", "collide_dimensions_not_in",
	"brightness", "current_generator", "tags", "draw_area", "activation_area",
	"fall_through_platforms", "has_feet",
	"x_schedule", "y_schedule", "rotation_schedule", "schedule_speed",
	"ctrl_up", "ctrl_down", "ctrl_left", "ctrl_right",
	"ctrl_attack", "ctrl_jump", "ctrl_tongue",
};
	ASSERT_EQ(NUM_CUSTOM_OBJECT_PROPERTIES, sizeof(CustomObjectProperties)/sizeof(*CustomObjectProperties));

	if(entries().empty()) {
		for(int n = 0; n != sizeof(CustomObjectProperties)/sizeof(*CustomObjectProperties); ++n) {
			entries().push_back(entry(CustomObjectProperties[n]));
		}

		for(int n = 0; n != entries().size(); ++n) {
			keys_to_slots()[entries()[n].id] = n;
		}
	}
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
	return get_key_slot(key);
}

game_logic::formula_callable_definition::entry* custom_object_callable::get_entry(int slot)
{
	if(slot < 0 || slot >= entries().size()) {
		return NULL;
	}

	return &entries()[slot];
}

const game_logic::formula_callable_definition::entry* custom_object_callable::get_entry(int slot) const
{
	if(slot < 0 || slot >= entries().size()) {
		return NULL;
	}

	return &entries()[slot];
}
