#include "controls.hpp"
#include "entity.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "joystick.hpp"
#include "player_info.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

player_info::player_info(entity& e, wml::const_node_ptr node)
  : entity_(&e),
	slot_(0)
{
	FOREACH_WML_CHILD(objects_node, node, "objects_destroyed") {
		std::vector<int>& v = objects_destroyed_[objects_node->attr("level")];
		v = vector_lexical_cast<int>(util::split(objects_node->attr("objects")));
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

void player_info::write(wml::node_ptr result) const
{
	for(std::map<std::string, std::vector<int> >::const_iterator i = objects_destroyed_.begin(); i != objects_destroyed_.end(); ++i) {
		get_objects_destroyed(i->first); //remove duplicates.

		wml::node_ptr objects(new wml::node("objects_destroyed"));
		objects->set_attr("level", i->first);
		std::ostringstream s;
		foreach(int n, i->second) {
			s << n << ",";
		}

		std::string str = s.str();
		if(str.empty() == false) {
			str.resize(str.size() - 1);
		}

		objects->set_attr("objects", str);
		result->add_child(objects);
	}
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
