#include <iostream>

#include "game_registry.hpp"
#include "json_parser.hpp"
#include "of_bridge.h"
#include "variant_utils.hpp"

game_registry& game_registry::instance()
{
	static game_registry* obj = new game_registry;
	static game_logic::formula_callable_ptr holder(obj);
	return *obj;
}

game_registry::game_registry()
{
	std::map<variant,variant> m;
	values_ = variant(&m);
}

void game_registry::set_contents(variant node)
{
	values_ = node;
	if(values_.is_null()) {
		*this = game_registry();
	}
}

variant game_registry::write_contents()
{
	return values_;
}

variant game_registry::get_value(const std::string& key) const
{
	return values_[variant(key)];
}

void game_registry::set_value(const std::string& key, const variant& value)
{
#if (TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE) && ENABLE_OPENFEINT
	try {
		variant leaderboards = json::parse_from_file("data/leaderboards.cfg");
		foreach(variant node, leaderboards["leaderboard"].as_list())
		{
			if(node["registry_key"].as_string() == key)
			{
				of_submit_score(node["of_id"].as_int(), value.as_int());
				break;
			}
		}
	} catch(json::parse_error&) {
	}
#endif
	values_ = values_.add_attr(variant(key), value);
}

