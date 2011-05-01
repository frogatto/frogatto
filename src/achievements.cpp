#include <algorithm>
#include <map>

#include "achievements.hpp"
#include "filesystem.hpp"
#include "formula_callable.hpp"
#include "i18n.hpp"
#include "preferences.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"
#include "of_bridge.h"
#include "variant.hpp"

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
  : id_(node->attr("id")), name_(i18n::tr(node->attr("name"))),
    description_(i18n::tr(node->attr("description"))),
	points_(wml::get_int(node, "points")),
	of_id_(wml::get_int(node, "of_id"))
{
}

namespace {
std::vector<std::string>* achievements = NULL;
}

bool attain_achievement(const std::string& id)
{
	if(achievements == NULL) {
		achievements = new std::vector<std::string>;
		variant val = preferences::registry()->query_value("achievements");
		if(val.is_string()) {
			*achievements = util::split(val.as_string());
			std::sort(achievements->begin(), achievements->end());
		}
	}

	if(std::binary_search(achievements->begin(), achievements->end(), id)) {
		return false;
	}
	
	#ifdef ENABLE_OPENFEINT
	achievement_ptr a = achievement::get(id);
	of_earn_achievement(a->of_id());
	#endif

	achievements->push_back(id);
	std::sort(achievements->begin(), achievements->end());

	preferences::registry()->mutate_value("achievements", variant(util::join(*achievements)));

	return true;
}
