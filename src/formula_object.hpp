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
#ifndef FORMULA_OBJECT_HPP_INCLUDED
#define FORMULA_OBJECT_HPP_INCLUDED

#include <set>
#include <string>
#include <vector>

#include <boost/function.hpp>

#include "formula.hpp"
#include "variant.hpp"
#include "wml_formula_callable.hpp"

namespace game_logic
{

class formula_class;

class formula_object : public game_logic::wml_serializable_formula_callable
{
public:
	static void visit_variants(variant v, boost::function<void (variant)> fn, std::vector<formula_object*>* seen=NULL);

	static void reload_classes();

	static boost::intrusive_ptr<formula_object> create(const std::string& type, variant args=variant());

	bool is_a(const std::string& class_name) const;

	//construct with data representing private/internal represenation.
	explicit formula_object(variant data);
	virtual ~formula_object();

	void validate() const;
private:
	//construct with type and constructor parameters.
	//Don't call directly, use create() instead.
	explicit formula_object(const std::string& type, variant args=variant());
	void call_constructors(variant args);

	variant serialize_to_wml() const;

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	void get_inputs(std::vector<formula_input>* inputs) const;

	//overrides of the class's read-only properties.
	std::map<std::string, formula_ptr> property_overrides_;

	boost::intrusive_ptr<const formula_class> class_;
	variant private_data_;

	variant tmp_value_;

	//if this is non-zero, then private_data_ will be exposed via get_value.
	mutable int expose_private_data_;
};

bool formula_class_valid(const std::string& type);

}

#endif
