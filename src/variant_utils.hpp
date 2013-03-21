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
#ifndef VARIANT_UTILS_HPP_INCLUDED
#define VARIANT_UTILS_HPP_INCLUDED

#include <boost/function.hpp>

#include <map>

#include "foreach.hpp"
#include "formula_callable.hpp"
#include "variant.hpp"

game_logic::formula_callable_ptr map_into_callable(variant v);

//Function which will try to append list of variations. Behaviors:
//anya, nullb -> anya
//nulla, anyb -> anyb
//lista, listb -> lista + listb
//anya, listb -> [anya] + listb
//lista, anyb -> lista + [anyb]
//obja, objb -> [obja, objb]
//scalara, scalarb -> scalarb
//lista, scalarb -> lista + [scalarb]
variant append_variants(variant a, variant b);

//parse a variant that may be a list of strings or a csv string. If null
//it will return an empty list.
std::vector<std::string> parse_variant_list_or_csv_string(variant v);

void merge_variant_over(variant* aptr, variant b);

void visit_variants(variant v, boost::function<void (variant)> fn);

variant deep_copy_variant(variant v);

template<typename Seq>
variant vector_to_variant(const Seq& seq) {
	std::vector<variant> v;
	foreach(const typename Seq::value_type& value, seq) {
		v.push_back(variant(value));
	}

	return variant(&v);
}

class variant_builder {
public:

	template<typename T>
	variant_builder& add(const std::string& name, const T& val) {
		return add_value(name, variant(val));
	}
	template<typename T>
	variant_builder& add(const std::string& name, T& val) {
		return add_value(name, variant(val));
	}
	variant_builder& add_value(const std::string& name, const variant& val);

	template<typename T>
	variant_builder& set(const std::string& name, const T& val) {
		return set_value(name, variant(val));
	}
	template<typename T>
	variant_builder& set(const std::string& name, T& val) {
		return set_value(name, variant(val));
	}
	variant_builder& set_value(const std::string& name, const variant& val);

	void merge_object(variant obj);

	variant build();
private:
	std::map<variant, std::vector<variant> > attr_;
};
#endif
