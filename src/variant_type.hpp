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
#pragma once
#ifndef VARIANT_TYPE_HPP_INCLUDED
#define VARIANT_TYPE_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "formula_tokenizer.hpp"
#include "variant.hpp"

class variant_type;
typedef boost::shared_ptr<const variant_type> variant_type_ptr;
typedef boost::shared_ptr<const variant_type> const_variant_type_ptr;

class variant_type
{
public:
	static variant_type_ptr get_any();
	static variant_type_ptr get_type(variant::TYPE type);
	static variant_type_ptr get_union(const std::vector<variant_type_ptr>& items);
	static variant_type_ptr get_list(variant_type_ptr element_type);
	static variant_type_ptr get_map(variant_type_ptr key_type, variant_type_ptr value_type);
	static variant_type_ptr get_class(const std::string& class_name);
	static variant_type_ptr get_function_type(const std::vector<variant_type_ptr>& arg_types, int min_args, variant_type_ptr return_type);

	virtual ~variant_type() {}
	virtual bool match(const variant& v) const = 0;

	virtual bool is_class(std::string* class_name=NULL) const { return false; }

	void set_str(const std::string& s) const { str_ = s; }
	const std::string& str() const { return str_; }

	virtual bool is_equal(const variant_type& o) const = 0;

private:
	virtual int order_id() const = 0;
	mutable std::string str_;
};

variant_type_ptr parse_variant_type(const variant& original_str,
                                    const formula_tokenizer::token*& i1,
                                    const formula_tokenizer::token* i2);
variant_type_ptr parse_variant_type(const variant& v);

#endif
