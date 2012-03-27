#include "foreach.hpp"
#include "formula_variable_storage.hpp"
#include "variant_utils.hpp"

namespace game_logic
{

formula_variable_storage::formula_variable_storage()
{}

formula_variable_storage::formula_variable_storage(const std::map<std::string, variant>& m)
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

}
