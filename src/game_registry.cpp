#include <iostream>

#include "game_registry.hpp"
#include "wml_node.hpp"

game_registry& game_registry::instance()
{
	static game_registry* obj = new game_registry;
	static game_logic::formula_callable_ptr holder(obj);
	return *obj;
}

void game_registry::set_contents(wml::const_node_ptr node)
{
	values_ = game_logic::map_formula_callable(node);
}

void game_registry::write_contents(wml::node_ptr node)
{
	values_.write(node);
}

variant game_registry::get_value(const std::string& key) const
{
	return values_.query_value(key);
}

void game_registry::set_value(const std::string& key, const variant& value)
{
	values_.mutate_value(key, value);
}

