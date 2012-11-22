/* $Id: formula.hpp 24955 2008-03-21 23:11:31Z dragonking $ */
/*
   Copyright (C) 2007 - 2008 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef FORMULA_HPP_INCLUDED
#define FORMULA_HPP_INCLUDED

#include <map>
#include <string>

#include "formula_callable_definition_fwd.hpp"
#include "formula_fwd.hpp"
#include "formula_function.hpp"
#include "formula_tokenizer.hpp"
#include "variant.hpp"

namespace game_logic
{

void set_verbatim_string_expressions(bool verbatim);

class formula_callable;
class formula_expression;
class function_symbol_table;
typedef boost::shared_ptr<formula_expression> expression_ptr;

//helper struct which contains info for a where expression.
struct where_variables_info : public reference_counted_object {
	explicit where_variables_info(int nslot) : base_slot(nslot) {}
	std::vector<std::string> names;
	std::vector<expression_ptr> entries;
	int base_slot;
};

typedef boost::intrusive_ptr<where_variables_info> where_variables_info_ptr;

class formula {
public:
	//a function which makes the current executing formula fail if
	//it's attempting to evaluate in a static context.
	static void fail_if_static_context();

	static variant evaluate(const const_formula_ptr& f,
	                    const formula_callable& variables,
						variant default_res=variant(0)) {
		if(f) {
			return f->execute(variables);
		} else {
			return default_res;
		}
	}

	static formula_ptr create_optional_formula(const variant& str, function_symbol_table* symbols=NULL, const formula_callable_definition* def=NULL);
	explicit formula(const variant& val, function_symbol_table* symbols=NULL, const formula_callable_definition* def=NULL);
	~formula();
	variant execute(const formula_callable& variables) const;
	variant execute() const;
	bool evaluates_to_constant(variant& result) const;
	std::string str() const { return str_.as_string(); }
	variant str_var() const { return str_; }

	std::string output_debug_info() const;

	bool has_guards() const { return base_expr_.empty() == false; }
	int guard_matches(const formula_callable& variables) const;

	//guard matches without wrapping 'variables' in the global callable.
	int raw_guard_matches(const formula_callable& variables) const;

	const_formula_callable_ptr wrap_callable_with_global_where(const formula_callable& callable) const;

	const expression_ptr& expr() const { return expr_; }

private:
	formula() {}
	variant str_;
	expression_ptr expr_;

	//for recursive function formulae, we have base cases along with
	//base expressions.
	struct BaseCase {
		//raw_guard is the guard without wrapping in the global where.
		expression_ptr raw_guard, guard, expr;
	};
	std::vector<BaseCase> base_expr_;

	where_variables_info_ptr global_where_;

	void check_brackets_match(const std::vector<formula_tokenizer::token>& tokens) const;

};

}

#endif
