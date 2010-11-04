#include <algorithm>
#include <map>

#include "achievements.hpp"
#include "filesystem.hpp"
#include "preferences.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

namespace {
std::map<std::string, achievement_ptr> cache;
}

achievement_ptr achievement::get(const std::string& id)
{
	if(cache.empty()) {
		wml::node_ptr node(wml::parse_wml_from_file("data/achievements.cfg"));
		if(node.get() == NULL) {
			cache[""]; //mark as loaded
			return achievement_ptr();
		}

		FOREACH_WML_CHILD(achievement_node, node, "achievement") {
			achievement_ptr a(new achievement(achievement_node));
			cache[a->id()] = a;
		}
	}

	return cache[id];
}

achievement::achievement(wml::const_node_ptr node)
  : id_(node->attr("id")), name_(node->attr("name")),
    description_(node->attr("description")),
	points_(wml::get_int(node, "points"))
{
}

namespace {
std::vector<std::string>* achievements = NULL;

std::string achievements_file() {
	return std::string(preferences::user_data_path()) + "/achievements.cfg";
}
}

bool attain_achievement(const std::string& id)
{
	if(achievements == NULL) {
		achievements = new std::vector<std::string>;
		if(!sys::read_file(achievements_file()).empty()) {
			wml::node_ptr node = wml::parse_wml_from_file(achievements_file());
			*achievements = util::split(node->attr("achievements"));
			std::sort(achievements->begin(), achievements->end());
		}
	}

	if(std::binary_search(achievements->begin(), achievements->end(), id)) {
		return false;
	}

	achievements->push_back(id);
	std::sort(achievements->begin(), achievements->end());

	wml::node_ptr node(new wml::node("achievements"));
	node->set_attr("achievements", util::join(*achievements));
	sys::write_file(achievements_file(), wml::output(node));

	return true;
}
