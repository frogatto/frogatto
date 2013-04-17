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

#include <map>

#include "formula_tokenizer.hpp"
#include "variant.hpp"

class variant_type;
typedef boost::shared_ptr<const variant_type> variant_type_ptr;
typedef boost::shared_ptr<const variant_type> const_variant_type_ptr;

class variant_type
{
public:
	static variant_type_ptr get_any();
	static variant_type_ptr get_commands();
	static variant_type_ptr get_type(variant::TYPE type);
	static variant_type_ptr get_union(const std::vector<variant_type_ptr>& items);
	static variant_type_ptr get_list(variant_type_ptr element_type);
	static variant_type_ptr get_map(variant_type_ptr key_type, variant_type_ptr value_type);
	static variant_type_ptr get_class(const std::string& class_name);
	static variant_type_ptr get_function_type(const std::vector<variant_type_ptr>& arg_types, variant_type_ptr return_type, int min_args);

	//get a version of the type that we now know isn't null.
	static variant_type_ptr get_null_excluded(variant_type_ptr input);

	variant_type();
	virtual ~variant_type();
	virtual bool match(const variant& v) const = 0;

	virtual bool is_any() const { return false; }
	virtual const std::vector<variant_type_ptr>* is_union() const { return NULL; }
	virtual variant_type_ptr is_list_of() const { return variant_type_ptr(); }
	virtual std::pair<variant_type_ptr,variant_type_ptr> is_map_of() const { return std::pair<variant_type_ptr,variant_type_ptr>(); }
	virtual bool is_type(variant::TYPE type) const { return false; }
	virtual bool is_class(std::string* class_name=NULL) const { return false; }

	virtual bool is_function(std::vector<variant_type_ptr>* args, variant_type_ptr* return_type, int* min_args) const { return false; }

	void set_str(const std::string& s) const { str_ = s; }
	const std::string& str() const { return str_; }

	virtual std::string to_string() const = 0;

	virtual bool is_equal(const variant_type& o) const = 0;

	virtual bool is_compatible(variant_type_ptr type) const { return false; }


private:
	virtual variant_type_ptr null_excluded() const { return variant_type_ptr(); }

	virtual int order_id() const = 0;
	mutable std::string str_;
};

std::string variant_type_is_class_or_null(variant_type_ptr type);

bool variant_types_compatible(variant_type_ptr to, variant_type_ptr from);

variant_type_ptr parse_variant_type(const variant& original_str,
                                    const formula_tokenizer::token*& i1,
                                    const formula_tokenizer::token* i2,
									bool allow_failure=false);
variant_type_ptr parse_variant_type(const variant& v);

variant_type_ptr
parse_optional_function_type(const variant& original_str,
                             const formula_tokenizer::token*& i1,
                             const formula_tokenizer::token* i2);
variant_type_ptr parse_optional_function_type(const variant& v);

variant_type_ptr
parse_optional_formula_type(const variant& original_str,
                            const formula_tokenizer::token*& i1,
                            const formula_tokenizer::token* i2);
variant_type_ptr parse_optional_formula_type(const variant& v);


#endif
