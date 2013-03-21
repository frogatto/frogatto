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
#include "asserts.hpp"
#include "foreach.hpp"
#include "formula_variable_storage.hpp"
#include "variant_utils.hpp"

namespace game_logic
{

formula_variable_storage::formula_variable_storage() : disallow_new_keys_(false)
{}

formula_variable_storage::formula_variable_storage(const std::map<std::string, variant>& m) : disallow_new_keys_(false)
{
	for(std::map<std::string, variant>::const_iterator i = m.begin(); i != m.end(); ++i) {
		add(i->first, i->second);
	}
}

bool formula_variable_storage::equal_to(const std::map<std::string, variant>& m) const
{
	if(m.size() != strings_to_values_.size()) {
		return false;
	}

	std::map<std::string, int>::const_iterator i = strings_to_values_.begin();
	std::map<std::string, variant>::const_iterator j = m.begin();

	while(i != strings_to_values_.end()) {
		if(i->first != j->first || j->second != values_[i->second]) {
			return false;
		}

		++i;
		++j;
	}

	return true;
}

void formula_variable_storage::read(variant node)
{
	if(node.is_null()) {
		return;
	}

	foreach(const variant_pair& val, node.as_map()) {
		add(val.first.as_string(), val.second);
	}
}

variant formula_variable_storage::write() const
{
	variant_builder node;
	for(std::map<std::string,int>::const_iterator i = strings_to_values_.begin(); i != strings_to_values_.end(); ++i) {
		node.add(i->first, values_[i->second]);
	}

	return node.build();
}

void formula_variable_storage::add(const std::string& key, const variant& value)
{
	std::map<std::string,int>::const_iterator i = strings_to_values_.find(key);
	if(i != strings_to_values_.end()) {
		values_[i->second] = value;
	} else {
		ASSERT_LOG(!disallow_new_keys_, "UNKNOWN KEY SET IN VAR STORAGE: " << key);
		strings_to_values_[key] = values_.size();
		values_.push_back(value);
	}
}

void formula_variable_storage::add(const formula_variable_storage& value)
{
	for(std::map<std::string, int>::const_iterator i = value.strings_to_values_.begin(); i != value.strings_to_values_.end(); ++i) {
		add(i->first, value.values_[i->second]);
	}
}

variant formula_variable_storage::get_value(const std::string& key) const
{
	std::map<std::string,int>::const_iterator i = strings_to_values_.find(key);
	if(i != strings_to_values_.end()) {
		return values_[i->second];
	} else {
		ASSERT_LOG(!disallow_new_keys_, "UNKNOWN KEY ACCESSED IN VAR STORAGE: " << key);
		return variant();
	}
}

variant formula_variable_storage::get_value_by_slot(int slot) const
{
	return values_[slot];
}

void formula_variable_storage::set_value(const std::string& key, const variant& value)
{
	add(key, value);
}

void formula_variable_storage::set_value_by_slot(int slot, const variant& value)
{
	values_[slot] = value;
}

void formula_variable_storage::get_inputs(std::vector<formula_input>* inputs) const
{
	for(std::map<std::string,int>::const_iterator i = strings_to_values_.begin(); i != strings_to_values_.end(); ++i) {
		inputs->push_back(formula_input(i->first, FORMULA_READ_WRITE));
	}
}

std::vector<std::string> formula_variable_storage::keys() const
{
	std::vector<std::string> result;
	for(std::map<std::string, int>::const_iterator i = strings_to_values_.begin(); i != strings_to_values_.end(); ++i) {
		result.push_back(i->first);
	}

	return result;
}

}
