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
#ifndef VARIANT_CALLABLE_HPP_INCLUDED
#define VARIANT_CALLABLE_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include "formula_callable.hpp"
#include "variant.hpp"

class variant_callable;

typedef boost::intrusive_ptr<variant_callable> variant_callable_ptr;
typedef boost::intrusive_ptr<const variant_callable> const_variant_callable_ptr;

class variant_callable : public game_logic::formula_callable
{
public:
	static variant create(variant* v);

	const variant& get_value() const { return value_; }
private:
	variant_callable(const variant& v);

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	variant create_for_list(const variant& list) const;

	variant value_;
};

#endif
