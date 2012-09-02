#include <map>
#include <string>
#include <vector>

#include "asserts.hpp"
#include "object_events.hpp"

namespace {
std::vector<std::string> create_object_event_names()
{
	std::vector<std::string> res;
	res.push_back("any");
	res.push_back("start_level");
	res.push_back("player_death");
	res.push_back("load");
	res.push_back("load_checkpoint");
	res.push_back("create");
	res.push_back("done_create");
	res.push_back("become_active");
	res.push_back("surface_damage");
	res.push_back("enter_anim");
	res.push_back("end_anim");
	res.push_back("collide_level");
	res.push_back("collide_head");
	res.push_back("collide_feet");
	res.push_back("collide_damage");
	res.push_back("collide_side");
	res.push_back("stuck");
	res.push_back("jumped_on");
	res.push_back("get_hit");
	res.push_back("process");
	res.push_back("timer");
	res.push_back("enter_water");
	res.push_back("exit_water");
	res.push_back("change_solid_dimensions_fail");
	res.push_back("add_object_fail");
	res.push_back("change_animation_failure");
	res.push_back("die");
	res.push_back("interact");
	res.push_back("child_spawned");
	res.push_back("spawned");
	res.push_back("draw");
	res.push_back("begin_dialog");
	res.push_back("cosmic_shift");
	res.push_back("schedule_finished");

	ASSERT_EQ(res.size(), NUM_OBJECT_BUILTIN_EVENT_IDS);
	return res;
}

std::vector<std::string>& object_event_names() {
	static std::vector<std::string> event_names = create_object_event_names();
	return event_names;
}

std::map<std::string, int> create_object_event_ids()
{
	std::map<std::string, int> result;
	for(int n = 0; n != object_event_names().size(); ++n) {
		result[object_event_names()[n]] = n;
	}

	return result;
}

std::map<std::string, int>& object_event_ids() {
	static std::map<std::string, int> event_ids = create_object_event_ids();
	return event_ids;
}

}

const std::string& get_object_event_str(int id)
{
	return object_event_names()[id];
}

int get_object_event_id(const std::string& str)
{
	std::map<std::string, int>::iterator itor = object_event_ids().find(str);
	if(itor != object_event_ids().end()) {
		return itor->second;
	}

	//we have to add a new entry for this new string
	object_event_ids()[str] = object_event_names().size();
	object_event_names().push_back(str);

	return object_event_names().size()-1;
}
