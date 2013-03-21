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
#include <vector>

#include "variant_callable.hpp"

variant variant_callable::create(variant* v)
{
	v->make_unique();
	return variant(new variant_callable(*v));
}

variant_callable::variant_callable(const variant& v) : value_(v)
{
}

variant variant_callable::get_value(const std::string& key) const
{
	if(key == "self" || key == "me") {
		return variant(this);
	}

	variant result = value_[variant(key)];
	if(result.is_list()) {
		return create_for_list(result);
	} else if(result.is_map()) {
		return variant(new variant_callable(result));
	} else {
		return result;
	}
}

variant variant_callable::create_for_list(const variant& value) const
{
	std::vector<variant> v;
	for(int n = 0; n != value.num_elements(); ++n) {
		const variant& item = value[n];
		if(item.is_list()) {
			v.push_back(create_for_list(item));
		} else if(item.is_map()) {
			v.push_back(variant(new variant_callable(item)));
		} else {
			v.push_back(item);
		}
	}

	return variant(&v);
}

void variant_callable::set_value(const std::string& key, const variant& value)
{
	value_.add_attr_mutation(variant(key), value);
}
