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
#ifndef CUSTOM_OBJECT_FUNCTIONS_HPP_INCLUDED
#define CUSTOM_OBJECT_FUNCTIONS_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <string>

#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_function.hpp"
#include "reference_counted_object.hpp"
#include "variant.hpp"

class custom_object;
class entity;
class level;

bool in_speech_dialog ();

using game_logic::function_symbol_table;
function_symbol_table& get_custom_object_functions_symbol_table();
void init_custom_object_functions(variant node);

class entity_command_callable : public game_logic::formula_callable {
public:
	entity_command_callable() : expr_(NULL) {}
	void run_command(level& lvl, entity& obj) const;

	void set_expression(const game_logic::formula_expression* expr);

	bool is_command() const { return true; }

private:
	virtual void execute(level& lvl, entity& ob) const = 0;
	variant get_value(const std::string& key) const { return variant(); }
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const {}

	//these two members are used as a more compiler-friendly version of a
	//intrusive_ptr<formula_expression>
	const game_logic::formula_expression* expr_;
	boost::intrusive_ptr<const reference_counted_object> expr_holder_;
};

class custom_object_command_callable : public game_logic::formula_callable {
public:
	custom_object_command_callable() : expr_(NULL) {}
	void run_command(level& lvl, custom_object& ob) const;

	void set_expression(const game_logic::formula_expression* expr);

	bool is_command() const { return true; }

private:
	virtual void execute(level& lvl, custom_object& ob) const = 0;
	variant get_value(const std::string& key) const { return variant(); }
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const {}
	
	//these two members are used as a more compiler-friendly version of a
	//intrusive_ptr<formula_expression>
	const game_logic::formula_expression* expr_;
	boost::intrusive_ptr<const reference_counted_object> expr_holder_;
};

class swallow_object_command_callable : public game_logic::formula_callable {
public:
	bool is_command() const { return true; }
private:
	variant get_value(const std::string& key) const { return variant(); }
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const {}
};

class swallow_mouse_command_callable : public game_logic::formula_callable {
public:
	bool is_command() const { return true; }
private:
	variant get_value(const std::string& key) const { return variant(); }
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const {}
};

#endif
