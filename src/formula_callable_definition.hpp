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
#ifndef FORMULA_CALLABLE_DEFINITION_HPP_INCLUDED
#define FORMULA_CALLABLE_DEFINITION_HPP_INCLUDED

#include <string>

#include "formula_callable_definition_fwd.hpp"

namespace game_logic
{

class formula_callable_definition
{
public:
	struct entry {
		explicit entry(const std::string& id_) : id(id_), type_definition(0) {}
		std::string id;
		const formula_callable_definition* type_definition;

		//optionally store the reference to the type definition.
		const_formula_callable_definition_ptr type_definition_holder;
	};

	formula_callable_definition() : is_strict_(false)
	{}
	virtual ~formula_callable_definition() {}

	virtual int get_slot(const std::string& key) const = 0;
	virtual entry* get_entry(int slot) = 0;
	virtual const entry* get_entry(int slot) const = 0;
	virtual int num_slots() const = 0;

	bool is_strict() const { return is_strict_; }
	void set_strict(bool value=true) { is_strict_ = value; }
private:
	bool is_strict_;
};

formula_callable_definition_ptr create_formula_callable_definition(const std::string* beg, const std::string* end, const formula_callable_definition* base=NULL);

int register_formula_callable_definition(const std::string& id, const formula_callable_definition* def);
const formula_callable_definition* get_formula_callable_definition(const std::string& id);

}

#endif
