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
typedef boost::shared_ptr<variant_type> variant_type_ptr;
typedef boost::shared_ptr<const variant_type> const_variant_type_ptr;

class variant_type
{
public:
	virtual ~variant_type() {}
	virtual bool match(const variant& v) const = 0;

	void set_str(const std::string& s) { str_ = s; }
	const std::string& str() const { return str_; }

private:
	std::string str_;
};

variant_type_ptr parse_variant_type(const variant& original_str,
                                    const formula_tokenizer::token*& i1,
                                    const formula_tokenizer::token* i2);
variant_type_ptr parse_variant_type(const variant& v);

#endif
