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
#include <map>
#include <vector>

#include "foreach.hpp"
#include "formula_callable_definition.hpp"
#include "formula_object.hpp"

namespace game_logic
{

void formula_callable_definition::entry::set_variant_type(variant_type_ptr type)
{
	variant_type = type;
	std::string class_name;
	if(type && type->is_class(&class_name)) {
		type_definition = get_class_definition(class_name);
	}
}

namespace
{

class simple_definition : public formula_callable_definition
{
public:
	simple_definition() : base_(NULL)
	{}

	int get_slot(const std::string& key) const {
		int index = 0;
		foreach(const entry& e, entries_) {
			if(e.id == key) {
				return base_num_slots() + index;
			}

			++index;
		}

		if(base_) {
			int result = base_->get_slot(key);
			if(result != -1) {
				return result;
			}
		}

		return -1;
	}

	entry* get_entry(int slot) {
		if(base_ && slot < base_num_slots()) {
			return const_cast<formula_callable_definition*>(base_)->get_entry(slot);
		}

		slot -= base_num_slots();

		if(slot < 0 || slot >= entries_.size()) {
			return NULL;
		}

		return &entries_[slot];
	}

	const entry* get_entry(int slot) const {
		if(base_ && slot < base_num_slots()) {
			return base_->get_entry(slot);
		}

		slot -= base_num_slots();

		if(slot < 0 || slot >= entries_.size()) {
			return NULL;
		}

		return &entries_[slot];
	}

	int num_slots() const { return base_num_slots() + entries_.size(); }

	void add(const std::string& id) {
		entries_.push_back(entry(id));
	}

	void add(const std::string& id, variant_type_ptr type) {
		entries_.push_back(entry(id));

		if(type) {
			entries_.back().variant_type = type;
			std::string class_name;
			if(type->is_class(&class_name)) {
				entries_.back().type_definition = get_class_definition(class_name);
			}
		}
	}

	void set_base(const formula_callable_definition* base) { base_ = base; }

private:
	int base_num_slots() const { return base_ ? base_->num_slots() : 0; }
	const formula_callable_definition* base_;
	std::vector<entry> entries_;
};

}

formula_callable_definition_ptr create_formula_callable_definition(const std::string* i1, const std::string* i2, const formula_callable_definition* base, variant_type_ptr* types)
{
	simple_definition* def = new simple_definition;
	def->set_base(base);
	while(i1 != i2) {

		if(types) {
			def->add(*i1, *types++);
		} else {
			def->add(*i1);
		}
		++i1;
	}

	return formula_callable_definition_ptr(def);
}

namespace {
std::map<std::string, const formula_callable_definition*> registry;
int num_definitions = 0;
}

int register_formula_callable_definition(const std::string& id, const formula_callable_definition* def)
{
	registry[id] = def;
	return ++num_definitions;
}

const formula_callable_definition* get_formula_callable_definition(const std::string& id)
{
	return registry[id];
}

}
