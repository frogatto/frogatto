/* $Id: formula_function.hpp 25643 2008-04-06 21:36:24Z ilor $ */
/*
   Copyright (C) 2008 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2
   or at your option any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef FORMULA_FUNCTION_HPP_INCLUDED
#define FORMULA_FUNCTION_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <map>

#include "formula_callable_definition_fwd.hpp"
#include "formula_callable_utils.hpp"
#include "formula_fwd.hpp"
#include "variant.hpp"

namespace game_logic {

class formula_expression;
typedef boost::shared_ptr<formula_expression> expression_ptr;

std::string pinpoint_location(variant v, std::string::const_iterator begin);
std::string pinpoint_location(variant v, std::string::const_iterator begin,
                                         std::string::const_iterator end);

class formula_expression {
public:
	explicit formula_expression(const char* name=NULL);
	virtual ~formula_expression() {}
	virtual variant static_evaluate(const formula_callable& variables) const {
		return evaluate(variables);
	}

	virtual bool is_identifier(std::string* id) const {
		return false;
	}

	virtual variant is_literal() const {
		return variant();
	}

	variant evaluate(const formula_callable& variables) const {
#if !TARGET_OS_IPHONE
		call_stack_manager manager(this);
#endif
		return execute(variables);
	}

	variant evaluate_with_member(const formula_callable& variables, std::string& id) const {
#if !TARGET_OS_IPHONE
		call_stack_manager manager(this);
#endif
		return execute_member(variables, id);
	}

	virtual expression_ptr optimize() const {
		return expression_ptr();
	}

	virtual bool can_reduce_to_variant(variant& v) const {
		return false;
	}

	virtual const formula_callable_definition* get_type_definition() const {
		return NULL;
	}

	const char* name() const { return name_; }
	void set_name(const char* name) { name_ = name; }

	void copy_debug_info_from(const formula_expression& o);
	virtual void set_debug_info(const variant& parent_formula,
	                            std::string::const_iterator begin_str,
	                            std::string::const_iterator end_str);
	bool has_debug_info() const;
	std::string debug_pinpoint_location() const;

	void set_str(const std::string& str) { str_ = str; }
	const std::string& str() const { return str_; }
protected:
	virtual variant execute_member(const formula_callable& variables, std::string& id) const;
private:
	virtual variant execute(const formula_callable& variables) const = 0;
	const char* name_;

	variant parent_formula_;
	std::string::const_iterator begin_str_, end_str_;
	std::string str_;
};

class function_expression : public formula_expression {
public:
	typedef std::vector<expression_ptr> args_list;
	explicit function_expression(
	                    const std::string& name,
	                    const args_list& args,
	                    int min_args=-1, int max_args=-1);

	virtual void set_debug_info(const variant& parent_formula,
	                            std::string::const_iterator begin_str,
	                            std::string::const_iterator end_str);

protected:
	const std::string& name() const { return name_; }
	const args_list& args() const { return args_; }
private:
	std::string name_;
	args_list args_;
	int min_args_, max_args_;
};

class formula_function_expression : public function_expression {
public:
	explicit formula_function_expression(const std::string& name, const args_list& args, const_formula_ptr formula, const_formula_ptr precondition, const std::vector<std::string>& arg_names);
	virtual ~formula_function_expression() {}

	void set_formula(const_formula_ptr f) { formula_ = f; }
private:
	variant execute(const formula_callable& variables) const;
	const_formula_ptr formula_;
	const_formula_ptr precondition_;
	std::vector<std::string> arg_names_;
	int star_arg_;

	//this is the callable object that is populated with the arguments to the
	//function. We try to reuse the same object every time the function is
	//called rather than recreating it each time.
	mutable boost::intrusive_ptr<slot_formula_callable> callable_;
};

typedef boost::shared_ptr<function_expression> function_expression_ptr;
typedef boost::shared_ptr<formula_function_expression> formula_function_expression_ptr;

class formula_function {
	std::string name_;
	const_formula_ptr formula_;
	const_formula_ptr precondition_;
	std::vector<std::string> args_;
public:
	formula_function() {}
	formula_function(const std::string& name, const_formula_ptr formula, const_formula_ptr precondition, const std::vector<std::string>& args) : name_(name), formula_(formula), precondition_(precondition), args_(args)
	{}

	formula_function_expression_ptr generate_function_expression(const std::vector<expression_ptr>& args) const;
};	

class function_symbol_table {
	std::map<std::string, formula_function> custom_formulas_;
	const function_symbol_table* backup_;
public:
	function_symbol_table() : backup_(0) {}
	virtual ~function_symbol_table() {}
	void set_backup(const function_symbol_table* backup) { backup_ = backup; }
	virtual void add_formula_function(const std::string& name, const_formula_ptr formula, const_formula_ptr precondition, const std::vector<std::string>& args);
	virtual expression_ptr create_function(const std::string& fn,
					                       const std::vector<expression_ptr>& args,
										   const formula_callable_definition* callable_def) const;
	std::vector<std::string> get_function_names() const;
};

//a special symbol table which is used to facilitate recursive functions.
//it is given to a formula function during parsing, and will give out
//function stubs for recursive calls. At the end of parsing it can fill
//in the real call.
class recursive_function_symbol_table : public function_symbol_table {
	std::string name_;
	formula_function stub_;
	function_symbol_table* backup_;
	mutable std::vector<formula_function_expression_ptr> expr_;
public:
	recursive_function_symbol_table(const std::string& fn, const std::vector<std::string>& args, function_symbol_table* backup);
	virtual expression_ptr create_function(const std::string& fn,
					                       const std::vector<expression_ptr>& args,
										   const formula_callable_definition* callable_def) const;
	void resolve_recursive_calls(const_formula_ptr f);
};

expression_ptr create_function(const std::string& fn,
                               const std::vector<expression_ptr>& args,
							   const function_symbol_table* symbols,
							   const formula_callable_definition* callable_def);
bool optimize_function_arguments(const std::string& fn,
                                 const function_symbol_table* symbols);
std::vector<std::string> builtin_function_names();

class variant_expression : public formula_expression {
public:
	explicit variant_expression(variant v) : formula_expression("_var"), v_(v)
	{}

	bool can_reduce_to_variant(variant& v) const {
		v = v_;
		return true;
	}

	variant is_literal() const {
		return v_;
	}
private:
	variant execute(const formula_callable& /*variables*/) const {
		return v_;
	}
	
	variant v_;
};

}

#endif
