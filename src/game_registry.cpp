/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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

