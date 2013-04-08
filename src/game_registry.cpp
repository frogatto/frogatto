#include <iostream>

#include "game_registry.hpp"
#include "json_parser.hpp"
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
	values_ = values_.add_attr(variant(key), value);
}

