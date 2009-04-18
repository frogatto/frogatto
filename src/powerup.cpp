#include <map>

#include "powerup.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
typedef std::map<std::string, const_powerup_ptr> powerup_map;
powerup_map cache;
}

powerup::powerup(wml::const_node_ptr node)
  : modifier_(node),
    icon_(new frame(node->get_child("icon"))),
	duration_(wml::get_int(node, "duration", -1)),
	permanent_(wml::get_bool(node, "permanent"))
{
}

void powerup::init(wml::const_node_ptr node)
{
	wml::node::const_child_iterator p1 = node->begin_child("powerup");
	wml::node::const_child_iterator p2 = node->end_child("powerup");
	for(; p1 != p2; ++p1) {
		cache[p1->second->attr("id")].reset(new powerup(p1->second));
	}
}

const_powerup_ptr powerup::get(const std::string& id)
{
	powerup_map::const_iterator itor = cache.find(id);
	if(itor == cache.end()) {
		return const_powerup_ptr();
	}

	return itor->second;
}
