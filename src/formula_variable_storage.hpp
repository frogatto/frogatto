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
#ifndef FORMULA_VARIABLE_STORAGE_HPP_INCLUDED
#define FORMULA_VARIABLE_STORAGE_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include "formula_callable.hpp"
#include "variant.hpp"

namespace game_logic
{

class formula_variable_storage : public formula_callable
{
public:
	formula_variable_storage();
	explicit formula_variable_storage(const std::map<std::string, variant>& m);

	bool equal_to(const std::map<std::string, variant>& m) const;

	void read(variant node);
	variant write() const;
	void add(const std::string& key, const variant& value);
	void add(const formula_variable_storage& value);

	std::vector<variant>& values() { return values_; }
	const std::vector<variant>& values() const { return values_; }

	std::vector<std::string> keys() const;

	void disallow_new_keys(bool value=true) { disallow_new_keys_ = value; }

private:
	variant get_value(const std::string& key) const;
	variant get_value_by_slot(int slot) const;
	void set_value(const std::string& key, const variant& value);
	void set_value_by_slot(int slot, const variant& value);

	void get_inputs(std::vector<formula_input>* inputs) const;
	
	std::vector<variant> values_;
	std::map<std::string, int> strings_to_values_;

	bool disallow_new_keys_;
};

typedef boost::intrusive_ptr<formula_variable_storage> formula_variable_storage_ptr;
typedef boost::intrusive_ptr<const formula_variable_storage> const_formula_variable_storage_ptr;

}

#endif
