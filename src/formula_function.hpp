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

#include <boost/shared_ptr.hpp>

#include <iostream>
#include <map>

#include "formula_fwd.hpp"
#include "variant.hpp"

namespace game_logic {

class formula_expression {
public:
	formula_expression() : name_(NULL) {}
	explicit formula_expression(const char* name) : name_(name) {}
	virtual ~formula_expression() {}
	variant evaluate(const formula_callable& variables) const {
		call_stack_manager manager(str_.c_str());
		return execute(variables);
	}

	variant evaluate_with_member(const formula_callable& variables, std::string& id) const {
		call_stack_manager manager(str_.c_str());
		return execute_member(variables, id);
	}
	void set_name(const char* name) { name_ = name; }
	void set_str(const std::string& str) { str_ = str; }
	const std::string& str() const { return str_; }
protected:
	virtual variant execute_member(const formula_callable& variables, std::string& id) const;
private:
	virtual variant execute(const formula_callable& variables) const = 0;
	const char* name_;
	std::string str_;
};

typedef boost::shared_ptr<formula_expression> expression_ptr;

class function_expression : public formula_expression {
public:
	typedef std::vector<expression_ptr> args_list;
	explicit function_expression(
	                    const std::string& name,
	                    const args_list& args,
	                    int min_args=-1, int max_args=-1);

protected:
	const args_list& args() const { return args_; }
private:
	std::string name_;
	args_list args_;
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
					                       const std::vector<expression_ptr>& args) const;
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
					                       const std::vector<expression_ptr>& args) const;
	void resolve_recursive_calls(const_formula_ptr f);
};

expression_ptr create_function(const std::string& fn,
                               const std::vector<expression_ptr>& args,
							   const function_symbol_table* symbols);
std::vector<std::string> builtin_function_names();

}

#endif
