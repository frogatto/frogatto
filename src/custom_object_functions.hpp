#ifndef CUSTOM_OBJECT_FUNCTIONS_HPP_INCLUDED
#define CUSTOM_OBJECT_FUNCTIONS_HPP_INCLUDED

#include <string>

#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_function.hpp"
#include "variant.hpp"

class custom_object;
class entity;
class level;

using game_logic::function_symbol_table;
function_symbol_table& get_custom_object_functions_symbol_table();

class entity_command_callable : public game_logic::formula_callable {
public:
	virtual void execute(level& lvl, entity& ob) const = 0;

private:
	variant get_value(const std::string& key) const { return variant(); }
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const {}
};

class custom_object_command_callable : public game_logic::formula_callable {
public:
	virtual void execute(level& lvl, custom_object& ob) const = 0;

private:
	variant get_value(const std::string& key) const { return variant(); }
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const {}
};

#endif
