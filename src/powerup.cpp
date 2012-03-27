#include <map>

#include "foreach.hpp"
#include "powerup.hpp"
#include "variant_utils.hpp"

namespace {
typedef std::map<std::string, const_powerup_ptr> powerup_map;
powerup_map cache;
}

powerup::powerup(variant node)
  : id_(node["id"].as_string()), modifier_(node),
    icon_(new frame(node["icon"])),
	duration_(node["duration"].as_int(-1)),
	permanent_(node["permanent"].as_bool())
{
}

void powerup::init(variant node)
{
	foreach(variant p, node["powerup"].as_list()) {
		cache[p["id"].as_string()].reset(new powerup(p));
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
