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
#ifndef WML_FORMULA_CALLABLE_HPP_INCLUDED
#define WML_FORMULA_CALLABLE_HPP_INCLUDED

#include <stdint.h>

#include <boost/intrusive_ptr.hpp>

#include "formula_callable.hpp"
#include "variant.hpp"

namespace game_logic
{

class wml_serializable_formula_callable : public formula_callable
{
public:
	explicit wml_serializable_formula_callable(bool has_self=true) : formula_callable(has_self) {}

	virtual ~wml_serializable_formula_callable() {}

	variant write_to_wml() const {
		return serialize_to_wml();
	}

	const std::string& addr() const { return addr_; }
protected:
	void set_addr(const std::string& addr) { addr_ = addr; }
private:
	virtual variant serialize_to_wml() const = 0;
	std::string addr_;
};

typedef boost::intrusive_ptr<wml_serializable_formula_callable> wml_serializable_formula_callable_ptr;
typedef boost::intrusive_ptr<const wml_serializable_formula_callable> const_wml_serializable_formula_callable_ptr;

class wml_formula_callable_serialization_scope
{
public:
	static void register_serialized_object(const_wml_serializable_formula_callable_ptr ptr);
	static bool is_active();

	wml_formula_callable_serialization_scope();
	~wml_formula_callable_serialization_scope();

	variant write_objects(variant obj) const;

private:
};

class wml_formula_callable_read_scope
{
public:
	static void register_serialized_object(intptr_t addr, wml_serializable_formula_callable_ptr ptr);
	static wml_serializable_formula_callable_ptr get_serialized_object(intptr_t addr);
	wml_formula_callable_read_scope();
	~wml_formula_callable_read_scope();

	static bool try_load_object(intptr_t id, variant& v);
private:
};

std::string serialize_doc_with_objects(variant v);
variant deserialize_doc_with_objects(const std::string& msg);

}

#endif
