#include "controls.hpp"
#include "entity.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "joystick.hpp"
#include "player_info.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

player_info::player_info(entity& e, variant node)
  : entity_(&e),
	slot_(0)
{
	foreach(variant objects_node, node["objects_destroyed"].as_list()) {
		std::vector<int>& v = objects_destroyed_[objects_node["level"].as_string()];
		v = objects_node["objects"].as_list_int();
	}
}

void player_info::object_destroyed(const std::string& level_id, int object)
{
	objects_destroyed_[level_id].push_back(object);
}

const std::vector<int>& player_info::get_objects_destroyed(const std::string& level_id) const
{
	std::vector<int>& v = objects_destroyed_[level_id];
	std::sort(v.begin(), v.end());
	v.erase(std::unique(v.begin(), v.end()), v.end());
	return v;
}

variant player_info::write() const
{
	variant_builder result;
	for(std::map<std::string, std::vector<int> >::const_iterator i = objects_destroyed_.begin(); i != objects_destroyed_.end(); ++i) {
		get_objects_destroyed(i->first); //remove duplicates.

		variant_builder objects;
		objects.add("level", i->first);

		objects.add("objects", vector_to_variant(i->second));

		result.add("objects_destroyed", objects.build());
	}

	return result.build();
}

void player_info::read_controls(int cycle)
{
	bool status[controls::NUM_CONTROLS];
	controls::get_control_status(cycle, slot_, status);

	if(status[controls::CONTROL_LEFT] && status[controls::CONTROL_RIGHT]) {
		//if both left and right are held, treat it as if neither are.
		status[controls::CONTROL_LEFT] = status[controls::CONTROL_RIGHT] = false;
	}

	for(int n = 0; n != controls::NUM_CONTROLS; ++n) {
		entity_->set_control_status(static_cast<controls::CONTROL_ITEM>(n), status[n]);
	}
}
