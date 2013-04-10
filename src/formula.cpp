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
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <stack>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <limits.h>


//#include "foreach.hpp"
#include "asserts.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_callable_definition.hpp"
#include "formula_constants.hpp"
#include "formula_function.hpp"
#include "formula_object.hpp"
#include "formula_tokenizer.hpp"
#include "i18n.hpp"
#include "map_utils.hpp"
#include "preferences.hpp"
#include "random.hpp"
#include "string_utils.hpp"
#include "unit_test.hpp"
#include "variant_type.hpp"
#include "variant_utils.hpp"

namespace {
	//the last formula that was executed; used for outputting debugging info.
	const game_logic::formula* last_executed_formula;

	bool _verbatim_string_expressions = false;

	int g_strict_formula_checking = 0;

}

std::string output_formula_error_info() {
	if(last_executed_formula) {
		return last_executed_formula->output_debug_info();
	}

	return "";
}

namespace game_logic
{

	void set_verbatim_string_expressions(bool verbatim) {
		_verbatim_string_expressions = verbatim;
	}
	
	void formula_callable::set_value(const std::string& key, const variant& /*value*/)
	{
		std::cerr << "ERROR: cannot set key '" << key << "' on object\n";
	}

	void formula_callable::set_value_by_slot(int slot, const variant& /*value*/)
	{
		std::cerr << "ERROR: cannot set slot '" << slot << "' on object\n";
	}

	variant formula_callable::get_value_by_slot(int slot) const
	{
		ASSERT_LOG(false, "Could not get value by slot from formula callable " << typeid(*this).name() << ": " << slot);
		return variant(0); //so VC++ doesn't complain
	}

	void formula_callable::serialize_to_string(std::string& str) const
	{
		if(preferences::serialize_bad_objects()) {
			//force serialization of this through so we can work out what's going on.
			str += "(UNSERIALIZABLE_OBJECT)";
			return;
		}

		throw type_error("Tried to serialize type which cannot be serialized");
	}

	bool formula_callable::execute_command(const variant &v) 
	{
		if(v.is_null()) {
			return true;
		}

		if(v.is_list()) {
			for(int n = 0; n != v.num_elements(); ++n) {
				execute_command(v[n]);
			}
		} else {
			command_callable* callable = v.try_convert<command_callable>();
			if(callable) {
				callable->run_command(*this);
			} else {
				ASSERT_LOG(false, "EXPECTED EXECUTABLE COMMAND OBJECT, INSTEAD FOUND: " << v.to_debug_string() << "\nFORMULA INFO: " << output_formula_error_info() << "\n");
			}
		}

		return true;
	}

	namespace {
	
	variant_type_ptr get_variant_type_from_value(const variant& value) {
		if(value.try_convert<formula_object>()) {
			return variant_type::get_class(value.try_convert<formula_object>()->get_class_name());
		} else if(value.is_list()) {
			std::vector<variant_type_ptr> types;
			foreach(const variant& item, value.as_list()) {
				variant_type_ptr new_type = get_variant_type_from_value(item);
				foreach(const variant_type_ptr& existing, types) {
					if(existing->is_equal(*new_type)) {
						new_type.reset();
						break;
					}
				}

				if(new_type) {
					types.push_back(new_type);
				}
			}

			return variant_type::get_list(variant_type::get_union(types));
		} else if(value.is_map()) {

			std::vector<variant_type_ptr> key_types, value_types;

			foreach(const variant::map_pair& p, value.as_map()) {
				variant_type_ptr new_key_type = get_variant_type_from_value(p.first);
				variant_type_ptr new_value_type = get_variant_type_from_value(p.second);

				foreach(const variant_type_ptr& existing, key_types) {
					if(existing->is_equal(*new_key_type)) {
						new_key_type.reset();
						break;
					}
				}

				if(new_key_type) {
					key_types.push_back(new_key_type);
				}

				foreach(const variant_type_ptr& existing, value_types) {
					if(existing->is_equal(*new_value_type)) {
						new_value_type.reset();
						break;
					}
				}

				if(new_value_type) {
					value_types.push_back(new_value_type);
				}
			}

			variant_type_ptr key_type, value_type;

			if(key_types.size() == 1) {
				key_type = variant_type::get_list(key_types[0]);
			} else {
				key_type = variant_type::get_list(variant_type::get_union(key_types));
			}

			if(value_types.size() == 1) {
				value_type = variant_type::get_list(value_types[0]);
			} else {
				value_type = variant_type::get_list(variant_type::get_union(value_types));
			}

			return variant_type::get_map(key_type, value_type);
		} else {
			return variant_type::get_type(value.type());
		}
	}

	}

	variant_type_ptr variant_expression::get_variant_type() const {
		return get_variant_type_from_value(v_);
	}

	command_callable::command_callable() : expr_(NULL)
	{
	}

	void command_callable::run_command(formula_callable& context) const
	{
		if(expr_) {
			try {
				fatal_assert_scope scope;
				execute(context);
			} catch(fatal_assert_failure_exception& e) {
				ASSERT_FATAL(e.msg << "\nERROR ENCOUNTERED WHILE RUNNING COMMAND GENERATED BY THIS EXPRESSION:\n" << expr_->debug_pinpoint_location());
			}
		} else {
			execute(context);
		}
	}

	void command_callable::set_expression(const formula_expression* expr) {
		expr_ = expr;
		expr_holder_.reset(expr);
	}

	formula_ptr formula_callable::create_formula(const variant& v) 
	{
		return formula_ptr(new formula(v, 0));
	}

	map_formula_callable::map_formula_callable(variant node)
	  : formula_callable(false), fallback_(NULL)
	{
		foreach(const variant_pair& value, node.as_map()) {
			values_[value.first.as_string()] = value.second;
		}
	}
	
	map_formula_callable::map_formula_callable(
											   const formula_callable* fallback) : formula_callable(false), fallback_(fallback)
	{}
	
	map_formula_callable::map_formula_callable(
											   const std::map<std::string, variant>& values) : formula_callable(false), fallback_(NULL), values_(values)
	{}
	
	map_formula_callable& map_formula_callable::add(const std::string& key,
													const variant& value)
	{
		values_[key] = value;
		return *this;
	}

	variant& map_formula_callable::add_direct_access(const std::string& key)
	{
		return values_[key];
	}
	
	variant map_formula_callable::get_value(const std::string& key) const
	{
		std::map<std::string, variant>::const_iterator itor = values_.find(key);
		if(itor == values_.end()) {
			if(fallback_) {
				return fallback_->query_value(key);
			} else {
				return variant();
			}
		} else {
			return itor->second;
		}
	}

	variant map_formula_callable::write() const
	{
		variant_builder result;
		for(std::map<std::string, variant>::const_iterator i = values_.begin();
		    i != values_.end(); ++i) {
			result.add(i->first, i->second);
		}
		return result.build();
	}
	
	void map_formula_callable::get_inputs(std::vector<formula_input>* inputs) const
	{
		if(fallback_) {
			fallback_->get_inputs(inputs);
		}
		for(std::map<std::string,variant>::const_iterator i = values_.begin(); i != values_.end(); ++i) {
			inputs->push_back(formula_input(i->first, FORMULA_READ_WRITE));
		}
	}
	
	void map_formula_callable::set_value(const std::string& key, const variant& value)
	{
		values_[key] = value;
	}
	
namespace {
		
class function_list_expression : public formula_expression {
public:
	explicit function_list_expression(function_symbol_table *symbols)
	: formula_expression("_function_list"), symbols_(symbols)
	{}

private:
	variant_type_ptr get_variant_type() const {
		return variant_type::get_list(variant_type::get_type(variant::VARIANT_TYPE_STRING));
	}
	variant execute(const formula_callable& /*variables*/) const {
		std::vector<variant> res;
		std::vector<std::string> function_names = builtin_function_names();
		std::vector<std::string> more_function_names = symbols_->get_function_names();
		function_names.insert(function_names.end(), more_function_names.begin(), more_function_names.end());
		for(size_t i = 0; i < function_names.size(); i++) {
			res.push_back(variant(function_names[i]));
		}
		return variant(&res);
	}
	
	function_symbol_table* symbols_;
};

class list_expression : public formula_expression {
public:
	explicit list_expression(const std::vector<expression_ptr>& items)
	: formula_expression("_list"), items_(items)
	{}

private:
	variant_type_ptr get_variant_type() const {
		std::vector<variant_type_ptr> types;
		foreach(const expression_ptr& item, items_) {
			variant_type_ptr new_type = item->query_variant_type();
			foreach(const variant_type_ptr& existing, types) {
				if(existing->is_equal(*new_type)) {
					new_type.reset();
					break;
				}
			}

			if(new_type) {
				types.push_back(new_type);
			}
		}

		if(types.size() == 1) {
			return variant_type::get_list(types[0]);
		} else {
			return variant_type::get_list(variant_type::get_union(types));
		}
	}

	//a special version of static evaluation that doesn't save a
	//reference to the list, so that we can allow static evaluation
	//not to be fooled.
	variant static_evaluate(const formula_callable& variables) const {
		std::vector<variant> res;
		res.reserve(items_.size());
		for(std::vector<expression_ptr>::const_iterator i = items_.begin(); i != items_.end(); ++i) {
			res.push_back((*i)->evaluate(variables));
		}

		return variant(&res);
	}

	variant execute(const formula_callable& variables) const {
		return static_evaluate(variables);
	}
	
	std::vector<expression_ptr> items_;
};

class list_comprehension_expression : public formula_expression {
public:
	list_comprehension_expression(expression_ptr expr, const std::map<std::string, expression_ptr>& generators, const std::vector<expression_ptr>& filters, int base_slot)
	  : formula_expression("_list_compr"), expr_(expr), generators_(generators), filters_(filters), base_slot_(base_slot)
	{
		for(std::map<std::string,expression_ptr>::const_iterator i = generators.begin(); i != generators.end(); ++i) {
			generator_names_.push_back(i->first);
		}
	}
	
private:
	variant_type_ptr get_variant_type() const {
		return variant_type::get_list(expr_->query_variant_type());
	}

	variant execute(const formula_callable& variables) const {
		std::vector<int> nelements;
		std::vector<variant> lists;
		for(std::map<std::string, expression_ptr>::const_iterator i = generators_.begin(); i != generators_.end(); ++i) {
			lists.push_back(i->second->evaluate(variables));
			nelements.push_back(lists.back().num_elements());
			if(nelements.back() == 0) {
				std::vector<variant> items;
				return variant(&items);
			}
		}

		std::vector<variant> result;

		boost::intrusive_ptr<slot_formula_callable> callable(new slot_formula_callable);
		callable->set_fallback(&variables);
		callable->set_base_slot(base_slot_);
		callable->reserve(generator_names_.size());
		std::vector<variant*> args;
		foreach(const std::string& arg, generator_names_) {
			callable->add(variant());
			args.push_back(&callable->back_direct_access());
		}

		std::vector<int> indexes(lists.size());
		for(;;) {
			for(int n = 0; n != indexes.size(); ++n) {
				*args[n] = lists[n][indexes[n]];
			}

			bool passes = true;
			foreach(const expression_ptr& filter, filters_) {
				if(filter->evaluate(*callable).as_bool() == false) {
					passes = false;
					break;
				}
			}

			if(passes) {
				result.push_back(expr_->evaluate(*callable));
			}

			if(!increment_vec(indexes, nelements)) {
				break;
			}
		}
		
		return variant(&result);
	}

	static bool increment_vec(std::vector<int>& v, const std::vector<int>& max_values) {
		int index = 0;
		while(index != v.size()) {
			if(++v[index] < max_values[index]) {
				return true;
			}

			v[index] = 0;
			++index;
		}

		return false;
	}

	expression_ptr expr_;
	std::map<std::string, expression_ptr> generators_;
	std::vector<std::string> generator_names_;
	std::vector<expression_ptr> filters_;
	int base_slot_;
};

class map_expression : public formula_expression {
public:
	explicit map_expression(const std::vector<expression_ptr>& items)
	: formula_expression("_map"), items_(items)
	{}
	
private:
	variant_type_ptr get_variant_type() const {
		std::vector<variant_type_ptr> key_types, value_types;
		for(std::vector<expression_ptr>::const_iterator i = items_.begin(); ( i != items_.end() ) && ( i+1 != items_.end() ) ; i+=2) {
			variant_type_ptr new_key_type = (*i)->query_variant_type();
			variant_type_ptr new_value_type = (*(i+1))->query_variant_type();

			foreach(const variant_type_ptr& existing, key_types) {
				if(existing->is_equal(*new_key_type)) {
					new_key_type.reset();
					break;
				}
			}

			if(new_key_type) {
				key_types.push_back(new_key_type);
			}

			foreach(const variant_type_ptr& existing, value_types) {
				if(existing->is_equal(*new_value_type)) {
					new_value_type.reset();
					break;
				}
			}

			if(new_value_type) {
				value_types.push_back(new_value_type);
			}
		}

		variant_type_ptr key_type, value_type;

		if(key_types.size() == 1) {
			key_type = variant_type::get_list(key_types[0]);
		} else {
			key_type = variant_type::get_list(variant_type::get_union(key_types));
		}

		if(value_types.size() == 1) {
			value_type = variant_type::get_list(value_types[0]);
		} else {
			value_type = variant_type::get_list(variant_type::get_union(value_types));
		}

		return variant_type::get_map(key_type, value_type);
	}

	variant execute(const formula_callable& variables) const {
		//since maps can be modified we want any map construction to return
		//a brand new map.
		formula::fail_if_static_context();

		std::map<variant,variant> res;
		for(std::vector<expression_ptr>::const_iterator i = items_.begin(); ( i != items_.end() ) && ( i+1 != items_.end() ) ; i+=2) {
			variant key = (*i)->evaluate(variables);
			variant value = (*(i+1))->evaluate(variables);
			res[ key ] = value;
		}
		
		variant result(&res);
		result.set_source_expression(this);
		return result;
	}
	
	std::vector<expression_ptr> items_;
};

class unary_operator_expression : public formula_expression {
public:
	unary_operator_expression(const std::string& op, expression_ptr arg)
	: formula_expression("_unary"), operand_(arg)
	{
		if(op == "not") {
			op_ = NOT;
		} else if(op == "-") {
			op_ = OP_SUB;
		} else {
			ASSERT_LOG(false, "illegal unary operator: '" << op << "'\n" << arg->debug_pinpoint_location());
		}
	}
private:
	variant_type_ptr get_variant_type() const {
		switch(op_) {
		case NOT: return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		case OP_SUB:
		default:
			if(operand_->query_variant_type()->is_type(variant::VARIANT_TYPE_INT)) {
				return variant_type::get_type(variant::VARIANT_TYPE_INT);
			} else {
				return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
			}
		}
	}

	variant execute(const formula_callable& variables) const {
		const variant res = operand_->evaluate(variables);
		switch(op_) {
			case NOT: 
				return res.as_bool() ? variant(0) : variant(1);
			case OP_SUB: 
			default: 
				return -res;
		}
	}
	enum OP { NOT, OP_SUB };
	OP op_;
	expression_ptr operand_;
};

class list_callable : public formula_callable {
	variant list_;
	
	list_callable(const list_callable&);
public:
	explicit list_callable(const variant& list) : formula_callable(false), list_(list)
	{}
	
	void get_inputs(std::vector<formula_input>* inputs) const {
		inputs->push_back(formula_input("size", FORMULA_READ_WRITE));
		inputs->push_back(formula_input("empty", FORMULA_READ_WRITE));
		inputs->push_back(formula_input("first", FORMULA_READ_WRITE));
		inputs->push_back(formula_input("last", FORMULA_READ_WRITE));
	}
	
	variant get_value(const std::string& key) const {
		if(key == "size") {
			return variant(unsigned(list_.num_elements()));
		} else if(key == "empty") {
			return variant(list_.num_elements() == 0);
		} else if(key == "first") {
			if(list_.num_elements() > 0) {
				return list_[0];
			} else {
				return variant();
			}
		} else if(key == "last") {
			if(list_.num_elements() > 0) {
				return list_[list_.num_elements()-1];
			} else {
				return variant();
			}
		} else {
			return variant();
		}
	}
};

class const_identifier_expression : public formula_expression {
public:
	explicit const_identifier_expression(const std::string& id)
	: formula_expression("_const_id"), v_(get_constant(id))
	{
	}
	
private:
	variant execute(const formula_callable& variables) const {
		return v_;
	}

	variant_type_ptr get_variant_type() const {
		return variant_type::get_type(v_.type());
	}
	
	variant v_;
};

class slot_identifier_expression : public formula_expression {
public:
	slot_identifier_expression(const std::string& id, int slot, const formula_callable_definition* callable_def)
	: formula_expression("_id"), slot_(slot), id_(id), callable_def_(callable_def)
	{
		ASSERT_LOG(callable_def_->get_entry(slot_) != NULL, "COULD NOT FIND DEFINITION IN SLOT CALLABLE: " << id);
		variant_type_ = callable_def_->get_entry(slot_)->variant_type;
	}
	
	const std::string& id() const { return id_; }

	bool is_identifier(std::string* ident) const {
		if(ident) {
			*ident = id_;
		}

		return true;
	}

	const formula_callable_definition* get_type_definition() const {
		const formula_callable_definition::entry* def = callable_def_->get_entry(slot_);
		ASSERT_LOG(def, "DID NOT FIND EXPECTED DEFINITION");
		if(def->type_definition) {
			return def->type_definition;
		} else {
			return formula_expression::get_type_definition();
		}
	}

	const variant_type_ptr& variant_type() const { return variant_type_; }
private:
	variant execute_member(const formula_callable& variables, std::string& id, variant* variant_id) const {
		id = id_;
		return variables.query_value("self");
	}
	
	variant execute(const formula_callable& variables) const {
		return variables.query_value_by_slot(slot_);
	}

	variant_type_ptr get_variant_type() const {
		return variant_type_;
	}

	int slot_;
	std::string id_;
	const formula_callable_definition* callable_def_;
	variant_type_ptr variant_type_;
};

class identifier_expression : public formula_expression {
public:
	identifier_expression(const std::string& id, const formula_callable_definition* callable_def)
	: formula_expression("_id"), id_(id), callable_def_(callable_def)
	{
	}
	
	const std::string& id() const { return id_; }

	bool is_identifier(std::string* ident) const {
		if(ident) {
			*ident = id_;
		}

		return true;
	}

	void set_function(expression_ptr fn) { function_ = fn; }

	expression_ptr optimize() const {
		if(callable_def_) {
			const int index = callable_def_->get_slot(id_);
			if(index != -1) {
				return expression_ptr(new slot_identifier_expression(id_, index, callable_def_));
			} else if(callable_def_->is_strict() || g_strict_formula_checking) {
				if(callable_def_->type_name() != NULL) {
					ASSERT_LOG(false, "Unknown symbol '" << id_ << "' in " << *callable_def_->type_name() << " " << debug_pinpoint_location());
				} else {
					ASSERT_LOG(false, "Unknown identifier '" << id_ << "' " << debug_pinpoint_location());
				}
			}
		}

		return expression_ptr();
	}

	const formula_callable_definition* get_type_definition() const {
		if(callable_def_) {
			const formula_callable_definition::entry* e = callable_def_->get_entry(callable_def_->get_slot(id_));
			if(e && e->type_definition) {
				return e->type_definition;
			} else {
				return formula_expression::get_type_definition();
			}
		}

		return NULL;
	}

private:
	variant execute_member(const formula_callable& variables, std::string& id, variant* variant_id) const {
		id = id_;
		return variables.query_value("self");
	}
	
	variant execute(const formula_callable& variables) const {
		variant result = variables.query_value(id_);
		if(result.is_null() && function_) {
			return function_->evaluate(variables);
		}

		return result;
	}
	variant_type_ptr get_variant_type() const {
		return variant_type::get_any();
	}
	std::string id_;
	const formula_callable_definition* callable_def_;

	//If this symbol is a function, this is the value we can return for it.
	expression_ptr function_;
};

class lambda_function_expression : public formula_expression {
public:
	lambda_function_expression(const std::vector<std::string>& args, const_formula_ptr fml, int base_slot, const std::vector<variant>& default_args, const std::vector<variant_type_ptr>& variant_types, const variant_type_ptr& return_type) : args_(args), fml_(fml), base_slot_(base_slot), default_args_(default_args), variant_types_(variant_types), return_type_(return_type)
	{
		if(!return_type_) {
			return_type_ = variant_type::get_any();
		}

		variant_types_.resize(args_.size());
		foreach(variant_type_ptr& t, variant_types_) {
			if(!t) {
				t = variant_type::get_any();
			}
		}
	}
	
private:
	variant execute(const formula_callable& variables) const {
		variant v(fml_, args_, variables, base_slot_, default_args_, variant_types_, return_type_);
		return v;
	}

	variant_type_ptr get_variant_type() const {
		return variant_type::get_function_type(variant_types_, return_type_, variant_types_.size() - default_args_.size());
	}
	
	std::vector<std::string> args_;
	game_logic::const_formula_ptr fml_;
	int base_slot_;
	std::vector<variant> default_args_;
	std::vector<variant_type_ptr> variant_types_;
	variant_type_ptr return_type_;
};

class function_call_expression : public formula_expression {
public:
	function_call_expression(expression_ptr left, const std::vector<expression_ptr>& args)
	: formula_expression("_fn"), left_(left), args_(args)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const variant left = left_->evaluate(variables);
		std::vector<variant> args;
		args.reserve(args_.size());
		foreach(const expression_ptr& e, args_) {
			args.push_back(e->evaluate(variables));
		}

		if(!left.is_function()) {
			//TODO: Nasty hack to make null() still work -- deprecated in
			//favor of null.
			if(left_->str() == "null" && args_.empty()) {
				return variant();
			}

			std::cerr << "ERROR: " << left_->str() << " IS NOT A VALID FUNCTION\n";
		}
		
		return left(args);
	}

	variant_type_ptr get_variant_type() const {
		variant_type_ptr return_type;
		if(left_->query_variant_type()->is_function(NULL, &return_type, NULL)) {
			return return_type;
		}

		return variant_type::get_any();
	}

	void static_error_analysis() const {
		variant_type_ptr fn_type = left_->query_variant_type();
		std::vector<variant_type_ptr> arg_types;
		int min_args = 0;
		const bool is_function = fn_type->is_function(&arg_types, NULL, &min_args);

	//TODO: make this function work.
#if 0
		//ASSERT_LOG(is_function, "FUNCTION CALL ON EXPRESSION WHICH ISN'T GUARANTEED TO BE A FUNCTION: " << fn_type->to_string() << " AT " << debug_pinpoint_location());
		if(!is_function) std::cerr << "XX: FUNCTION CALL ON EXPRESSION WHICH ISN'T GUARANTEED TO BE A FUNCTION: " << fn_type->to_string() << " at '" << str() << "' " << debug_pinpoint_location() << "\n";

		if(is_function) {
			for(int n = 0; n != args_.size() && n != arg_types.size(); ++n) {
				variant_type_ptr t = args_[n]->query_variant_type();
				std::cerr << "MATCH: " << t->to_string() << " WITH " << arg_types[n]->to_string() << "\n";
				if(variant_types_compatible(arg_types[n], t) == false) {
					std::cerr << "YY: FUNCTION CALL DOES NOT MATCH: " << debug_pinpoint_location() << " ARGUMENT " << (n+1) << " TYPE " << t->to_string() << " DOES NOT MATCH " << arg_types[n]->to_string() << "\n";
				}

				std::cerr << "DONE MATCH\n";
			}
		}
#endif
	}
	
	expression_ptr left_;
	std::vector<expression_ptr> args_;
};

class dot_expression : public formula_expression {
public:
	dot_expression(expression_ptr left, expression_ptr right)
	: formula_expression("_dot"), left_(left), right_(right)
	{}
	const formula_callable_definition* get_type_definition() const {
		return right_->get_type_definition();
	}
private:
	variant execute(const formula_callable& variables) const {
		const variant left = left_->evaluate(variables);
		if(!left.is_callable()) {
			if(left.is_list()) {
				formula_callable_ptr lc(new list_callable(left));	
				return right_->evaluate(*lc);
			} else if(left.is_map()) {
				return left[variant(right_->str())];
			}

			ASSERT_LOG(!left.is_null(), "CALL OF DOT OPERATOR ON NULL VALUE: '" << left_->str() << "': " << debug_pinpoint_location());
			ASSERT_LOG(false, "CALL OF DOT OPERATOR ON ILLEGAL VALUE: " << left.write_json() << " PRODUCED BY '" << left_->str() << "': " << debug_pinpoint_location());
			
			return left;
		}
		
		return right_->evaluate(*left.as_callable());
	}
	
	variant execute_member(const formula_callable& variables, std::string& id, variant* variant_id) const {
		variant left = left_->evaluate(variables);
		
		if(!right_->is_identifier(&id)) {
			return right_->evaluate_with_member(*left.as_callable(), id);
		}
		
		return left;
	}

	variant_type_ptr get_variant_type() const {
		return right_->query_variant_type();
	}
	
	expression_ptr left_, right_;
};

class square_bracket_expression : public formula_expression { //TODO
public:
	square_bracket_expression(expression_ptr left, expression_ptr key)
	: formula_expression("_sqbr"), left_(left), key_(key)
	{
	}
private:
	variant execute(const formula_callable& variables) const {
		const variant left = left_->evaluate(variables);
		const variant key = key_->evaluate(variables);
		if(left.is_list() || left.is_map()) {
			return left[ key ];
		} else if(left.is_string()) {
			const std::string& s = left.as_string();
			int index = key.as_int();
			ASSERT_LOG(index < s.length(), "index outside bounds: " << s << "[" << index << "]'\n'"  << debug_pinpoint_location());
			return variant(s.substr(index, 1));
		} else if(left.is_callable()) {
			return left.as_callable()->query_value(key.as_string());
		} else {
			std::cerr << "STACK TRACE FOR ERROR:\n" << get_call_stack() << "\n";
			std::cerr << output_formula_error_info();
			ASSERT_LOG(false, "illegal usage of operator []: called on " << left.to_debug_string() << " value: " << left_->str() << "'\n" << debug_pinpoint_location());
		}
	}
	
	variant execute_member(const formula_callable& variables, std::string& id, variant* variant_id) const {
		const variant left = left_->evaluate(variables);
		const variant key = key_->evaluate(variables);

		if(key.is_string()) {
			id = key.as_string();
		} else if(variant_id) {
			*variant_id = key;
		}
		return left;
	}

	variant_type_ptr get_variant_type() const {
		variant_type_ptr left_type = left_->query_variant_type();
		variant_type_ptr list_element_type = left_type->is_list_of();
		if(list_element_type) {
			return list_element_type;
		}

		std::pair<variant_type_ptr, variant_type_ptr> p = left_type->is_map_of();
		if(p.second) {
			return p.second;
		}

		return variant_type::get_any();
	}
	
	expression_ptr left_, key_;
};

class slice_square_bracket_expression : public formula_expression {
public:
	slice_square_bracket_expression(expression_ptr left, expression_ptr start, expression_ptr end)
	: formula_expression("_slice_sqbr"), left_(left), start_(start), end_(end)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const variant left = left_->evaluate(variables);
		int begin_index = start_ ? start_->evaluate(variables).as_int() : 0;
		int end_index = end_ ? end_->evaluate(variables).as_int() : left.num_elements();

		if(left.is_string()) {
			const std::string& s = left.as_string();
			if(begin_index > s.length()) {
				begin_index = s.length();
			}
			if(end_index > s.length()) {
				end_index = s.length();
			}
			if(s.length() == 0) {
				return variant();
			}
			if(end_index >= begin_index) {
				return variant(s.substr(begin_index, end_index-begin_index));
			} else {
				return variant();
			}
		}

		if(begin_index > left.num_elements()) {
			begin_index = left.num_elements();
		}

		if(end_index > left.num_elements()) {
			end_index = left.num_elements();
		}
		
		if(left.is_list()) {
			if(left.num_elements() == 0) {
				return variant();
			}
			if(end_index >= begin_index) {
				return left.get_list_slice(begin_index, end_index);
			} else {
				return variant();
			}
			
		} else {
			ASSERT_LOG(false, "illegal usage of operator [:]'\n" << debug_pinpoint_location() << " called on object of type " << variant::variant_type_to_string(left.type()));
		}
	}

	variant_type_ptr get_variant_type() const {
		return left_->query_variant_type();
	}
	
	expression_ptr left_, start_, end_;
};

variant_type_ptr get_variant_type_and_or(expression_ptr left, expression_ptr right) {
	variant_type_ptr left_type = left->query_variant_type();
	variant_type_ptr right_type = right->query_variant_type();
	if(left_type->is_equal(*right_type)) {
		return left_type;
	}

	std::vector<variant_type_ptr> types;
	types.push_back(left_type);
	types.push_back(right_type);
	return variant_type::get_union(types);
}

class and_operator_expression : public formula_expression {
public:
	and_operator_expression(expression_ptr left, expression_ptr right)
	  : formula_expression("_and"), left_(left), right_(right)
	{
	}

private:
	variant execute(const formula_callable& variables) const {
		variant v = left_->evaluate(variables);
		if(!v.as_bool()) {
			return v;
		}

		return right_->evaluate(variables);
	}

	variant_type_ptr get_variant_type() const {
		return get_variant_type_and_or(left_, right_);
	}

	expression_ptr left_, right_;
};

class or_operator_expression : public formula_expression {
public:
	or_operator_expression(expression_ptr left, expression_ptr right)
	  : formula_expression("_or"), left_(left), right_(right)
	{
	}

private:
	variant execute(const formula_callable& variables) const {
		variant v = left_->evaluate(variables);
		if(v.as_bool()) {
			return v;
		}

		return right_->evaluate(variables);
	}

	variant_type_ptr get_variant_type() const {
		return get_variant_type_and_or(left_, right_);
	}

	expression_ptr left_, right_;
};

class operator_expression : public formula_expression {
public:
	operator_expression(const std::string& op, expression_ptr left,
						expression_ptr right)
	: formula_expression("_op"), op_(OP(op[0])), left_(left), right_(right)
	{
		if(op == ">=") {
			op_ = OP_GTE;
		} else if(op == "<=") {
			op_ = OP_LTE;
		} else if(op == "!=") {
			op_ = OP_NEQ;
		} else if(op == "and") {
			op_ = OP_AND;
		} else if(op == "or") {
			op_ = OP_OR;
		} else if(op == "in") {
			op_ = OP_IN;
		} else if(op == "not in") {
			op_ = OP_NOT_IN;
		}
	}

	expression_ptr optimize() const {
		if(op_ == OP_AND) {
			return expression_ptr(new and_operator_expression(left_, right_));
		} else if(op_ == OP_OR) {
			return expression_ptr(new or_operator_expression(left_, right_));
		}

		return expression_ptr();
	}

	expression_ptr get_left() const { return left_; }
	expression_ptr get_right() const { return right_; }
	
private:
	variant execute(const formula_callable& variables) const {
		const variant left = left_->evaluate(variables);
		variant right = right_->evaluate(variables);
		switch(op_) {
			case OP_IN:
			case OP_NOT_IN: {
				bool result = op_ == OP_IN;
				if(right.is_list()) {
					for(int n = 0; n != right.num_elements(); ++n) {
						if(left == right[n]) {
							return variant::from_bool(result);
						}
					}
					
					return variant::from_bool(!result);
				} else if(right.is_map()) {
					return variant(right.has_key(left) ? result : !result);
				} else {
					ASSERT_LOG(false, "ILLEGAL OPERAND TO 'in': " << right.write_json() << " AT " << debug_pinpoint_location());
					return variant();
				}
			}
			case OP_AND: 
				return left.as_bool() == false ? left : right;
			case OP_OR: 
				return left.as_bool() ? left : right;
			case OP_ADD: 
				return left + right;
			case OP_SUB: 
				return left - right;
			case OP_MUL: 
				return left * right;
			case OP_DIV: 

				//this is a very unorthodox hack to guard against divide-by-zero errors.  It returns positive or negative infinity instead of asserting, which (hopefully!) works out for most of the physical calculations that are using this.  We tentatively view this behavior as much more preferable to the game apparently crashing for a user.  This is of course not rigorous outside of a videogame setting.
				if(right == variant(0)) { 
					right = variant(decimal::epsilon());
				}

				return left / right;
			case OP_POW: 
				return left ^ right;
			case OP_EQ:  
				return left == right ? variant::from_bool(true) : variant::from_bool(false);
			case OP_NEQ: 
				return left != right ? variant::from_bool(true) : variant::from_bool(false);
			case OP_LTE: 
				return left <= right ? variant::from_bool(true) : variant::from_bool(false);
			case OP_GTE: 
				return left >= right ? variant::from_bool(true) : variant::from_bool(false);
			case OP_LT:  
				return left < right ? variant::from_bool(true) : variant::from_bool(false);
			case OP_GT:  
				return left > right ? variant::from_bool(true) : variant::from_bool(false);
			case OP_MOD: 
				return left % right;
			case OP_DICE:
			default:
				return variant(dice_roll(left.as_int(), right.as_int()));
		}
	}
	
	static int dice_roll(int num_rolls, int faces) {
		int res = 0;
		while(faces > 0 && num_rolls-- > 0) {
			res += (rng::generate()%faces)+1;
		}
		return res;
	}

	variant_type_ptr get_variant_type() const {
		switch(op_) {
		case OP_IN:
		case OP_NOT_IN:
		case OP_NEQ:
		case OP_LTE:
		case OP_GTE:
		case OP_GT:
		case OP_LT:
		case OP_EQ:
			return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		case OP_AND:
		case OP_OR: {
			variant_type_ptr left_type = left_->query_variant_type();
			variant_type_ptr right_type = left_->query_variant_type();
			if(left_type->is_equal(*right_type)) {
				return left_type;
			}

			std::vector<variant_type_ptr> v;
			v.push_back(left_type);
			v.push_back(right_type);
			return variant_type::get_union(v);
		}

		case OP_ADD: {
			variant_type_ptr left_type = left_->query_variant_type();
			variant_type_ptr right_type = right_->query_variant_type();
			if(left_type->is_equal(*right_type)) {
				return left_type;
			}

			if(left_type->is_type(variant::VARIANT_TYPE_STRING) || right_type->is_type(variant::VARIANT_TYPE_STRING)) {
				return left_type;
			}

			if(left_type->is_type(variant::VARIANT_TYPE_DECIMAL) || right_type->is_type(variant::VARIANT_TYPE_DECIMAL)) {
				return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
			}

			//TODO: improve this, handle remaining cases!
			return variant_type::get_any();
		}

		case OP_MUL: {
			variant_type_ptr left_type = left_->query_variant_type();
			variant_type_ptr right_type = right_->query_variant_type();
			//TODO: detect lists here!
			return variant_type::get_any();
		}

		case OP_POW:
		case OP_DIV:
		case OP_SUB: {
			variant_type_ptr left_type = left_->query_variant_type();
			variant_type_ptr right_type = right_->query_variant_type();
			if(left_type->is_type(variant::VARIANT_TYPE_DECIMAL) || right_type->is_type(variant::VARIANT_TYPE_DECIMAL)) {
				return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
			}

			return variant_type::get_type(variant::VARIANT_TYPE_INT);
		}

		case OP_MOD:
		case OP_DICE:
			return variant_type::get_type(variant::VARIANT_TYPE_INT);
		default:
			ASSERT_LOG(false, "unknown op type: " << op_);
			
		}
	}
	
	enum OP { OP_IN, OP_NOT_IN, OP_AND, OP_OR, OP_NEQ, OP_LTE, OP_GTE, OP_GT='>', OP_LT='<', OP_EQ='=',
		OP_ADD='+', OP_SUB='-', OP_MUL='*', OP_DIV='/', OP_DICE='d', OP_POW='^', OP_MOD='%' };
	
	OP op_;
	expression_ptr left_, right_;
};

typedef std::map<std::string,expression_ptr> expr_table;
typedef boost::shared_ptr<expr_table> expr_table_ptr;

const_formula_callable_definition_ptr create_where_definition(expr_table_ptr table, const formula_callable_definition* def)
{
	std::vector<std::string> items;
	std::vector<variant_type_ptr> types;
	for(std::map<std::string,expression_ptr>::const_iterator i = table->begin(); i != table->end(); ++i) {
		items.push_back(i->first);
		types.push_back(i->second->query_variant_type());
	}

	ASSERT_LOG(items.empty() == false, "EMPTY WHERE CLAUSE");

	return create_formula_callable_definition(&items[0], &items[0] + items.size(), def, &types[0]);
}

class where_variables: public formula_callable {
public:
	where_variables(const formula_callable &base, where_variables_info_ptr info)
	: formula_callable(false), base_(&base), info_(info)
	{}
private:
	boost::intrusive_ptr<const formula_callable> base_;
	where_variables_info_ptr info_;
	
	mutable std::vector<variant> results_cache_;

	variant get_value_by_slot(int slot) const {
		if(slot >= info_->base_slot) {
			slot -= info_->base_slot;
			if(slot < results_cache_.size() && results_cache_[slot].is_null() == false) {
				return results_cache_[slot];
			} else {
				variant result = info_->entries[slot]->evaluate(*base_);
				if(results_cache_.size() <= slot) {
					results_cache_.resize(slot+1);
				}

				results_cache_[slot] = result;
				return result;
			}
		}

		return base_->query_value_by_slot(slot);
	}
	
	variant get_value(const std::string& key) const {
		const variant result = base_->query_value(key);
		if(result.is_null()) {
			std::vector<std::string>::const_iterator i = std::find(info_->names.begin(), info_->names.end(), key);
			if(i != info_->names.end()) {
				const int slot = i - info_->names.begin();
				return get_value_by_slot(info_->base_slot + slot);
			}
		}
		return result;
	}
};

class where_expression : public formula_expression {
public:
	where_expression(expression_ptr body, where_variables_info_ptr info)
	: formula_expression("_where"), body_(body), info_(info)
	{
	}
	
private:
	variant_type_ptr get_variant_type() const {
		return body_->query_variant_type();
	}

	expression_ptr body_;
	where_variables_info_ptr info_;
	
	variant execute(const formula_callable& variables) const {
		formula_callable_ptr wrapped_variables(new where_variables(variables, info_));
		return body_->evaluate(*wrapped_variables);
	}
};

class type_expression : public formula_expression {
public:
	type_expression(variant_type_ptr type, expression_ptr expr)
	: formula_expression("_type"), type_(type), expression_(expr)
	{
	}
	
private:
	variant_type_ptr get_variant_type() const {
		return type_;
	}

	variant_type_ptr type_;
	expression_ptr expression_;
	
	variant execute(const formula_callable& variables) const {
		const variant result = expression_->evaluate(variables);
		ASSERT_LOG(type_->match(result), "TYPE MIS-MATCH: EXPECTED " << type_->to_string() << " BUT FOUND " << result.write_json() << " AT " << debug_pinpoint_location());
		return result;
	}
};

class assert_expression : public formula_expression {
public:
	assert_expression(expression_ptr body, const std::vector<expression_ptr> asserts, expression_ptr debug_expr)
	  : formula_expression("_assert"), body_(body), asserts_(asserts), debug_(debug_expr)
	{
	}

private:
	expression_ptr body_, debug_;
	std::vector<expression_ptr> asserts_;

	variant execute(const formula_callable& variables) const {
		foreach(const expression_ptr& a, asserts_) {
			if(!a->evaluate(variables).as_bool()) {
				operator_expression* op_expr = dynamic_cast<operator_expression*>(a.get());

				std::ostringstream expr_info;
				if(op_expr) {
					expr_info << "  " << op_expr->get_left()->str() << ": " << op_expr->get_left()->evaluate(variables).to_debug_string() << "\n";
					expr_info << "  " << op_expr->get_right()->str() << ": " << op_expr->get_right()->evaluate(variables).to_debug_string() << "\n";
				}

				if(debug_) {
					expr_info << "DEBUG EXPRESSION: " << debug_->str() << " -> " << debug_->evaluate(variables).to_debug_string() << "\n";
				}

				ASSERT_LOG(false,
			               "FORMULA ASSERTION FAILED: " << a->str() << " -- " << a->debug_pinpoint_location() << "\n" << expr_info.str());
			}
		}

		return body_->evaluate(variables);
	}

	variant_type_ptr get_variant_type() const {
		return body_->query_variant_type();
	}
};

class null_expression : public formula_expression {
public:
	explicit null_expression() : formula_expression("_null") {}
private:
	variant execute(const formula_callable& /*variables*/) const {
		return variant();
	}

	variant_type_ptr get_variant_type() const {
		return variant_type::get_type(variant::VARIANT_TYPE_NULL);
	}
};


class integer_expression : public formula_expression {
public:
	explicit integer_expression(int i) : formula_expression("_int"), i_(i)
	{}
private:
	variant execute(const formula_callable& /*variables*/) const {
		return i_;
	}

	variant_type_ptr get_variant_type() const {
		return variant_type::get_type(variant::VARIANT_TYPE_INT);
	}
	
	variant i_;
};

class decimal_expression : public formula_expression {
public:
	explicit decimal_expression(const decimal& d) : formula_expression("_decimal"), v_(d)
	{}
private:
	variant execute(const formula_callable& /*variables*/) const {
		return v_;
	}

	variant_type_ptr get_variant_type() const {
		return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
	}
	
	variant v_;
};

class string_expression : public formula_expression {
public:
	explicit string_expression(std::string str, bool translate = false, function_symbol_table* symbols = 0) : formula_expression("_string")
	{
		if (!_verbatim_string_expressions) {
			const std::string original = str;

			size_t pos = 0;
			//replace \\n sequences with newlines
			while((pos = str.find("\\n", pos)) != std::string::npos) {
				str = str.replace(pos, 2, "\n");
			}
			pos = 0;

			str.erase(std::remove(str.begin(), str.end(), '\t'), str.end());

			if (translate) {
				str = i18n::tr(str);
			}

			static const std::string BeginSub = "${";
			std::string::iterator i;
			while((i = std::search(str.begin(), str.end(), BeginSub.begin(), BeginSub.end())) != str.end()) {
				std::string::iterator j = std::find(i, str.end(), '}');
				if(j == str.end()) {
					break;
				}
			
				const std::string formula_str(i+BeginSub.size(), j);
				const int pos = i - str.begin();
				str.erase(i, j+1);
			
				substitution sub;
				sub.pos = pos;
				sub.calculation.reset(new formula(variant(formula_str), symbols));
				subs_.push_back(sub);
			}
		
			std::reverse(subs_.begin(), subs_.end());

			if(translate) {
				str_ = variant::create_translated_string(original, str);
				return;
			}
		} else if (translate) {
			str = std::string("~") + str + std::string("~");
		}
		
		str_ = variant(str);
	}

	variant is_literal() const {
		if(subs_.empty()) {
			return str_;
		} else {
			return variant();
		}
	}
private:
	variant execute(const formula_callable& variables) const {
		if(subs_.empty()) {
			return str_;
		} else {
			std::string res = str_.as_string();
			for(size_t i=0; i < subs_.size(); ++i) {
				const substitution& sub = subs_[i];
				const std::string str = sub.calculation->execute(variables).string_cast();
				res.insert(sub.pos, str);
			}
			
			return variant(res);
		}
	}

	variant_type_ptr get_variant_type() const {
		return variant_type::get_type(variant::VARIANT_TYPE_STRING);
	}
	
	struct substitution {
		int pos;
		const_formula_ptr calculation;
	};
	
	variant str_;
	std::vector<substitution> subs_;
};

using namespace formula_tokenizer;
int operator_precedence(const token& t)
{
	static std::map<std::string,int> precedence_map;
	if(precedence_map.empty()) {
		int n = 0;
		precedence_map["->"] = ++n;
		precedence_map["where"] = ++n;
		precedence_map["asserting"] = ++n;
		precedence_map["<-"] = ++n;
		precedence_map["not"] = ++n;
		precedence_map["or"]    = ++n;
		precedence_map["and"]   = ++n;
		precedence_map["in"] = ++n;
		precedence_map["="]     = ++n;
		precedence_map["!="]    = n;
		precedence_map["<"]     = n;
		precedence_map[">"]     = n;
		precedence_map["<="]    = n;
		precedence_map[">="]    = n;
		precedence_map["+"]     = ++n;
		precedence_map["-"]     = n;
		precedence_map["*"]     = ++n;
		precedence_map["/"]     = ++n;
		precedence_map["%"]     = ++n;
		precedence_map["^"]     = ++n;
		precedence_map["d"]     = ++n;

		//these operators are equal precedence, and left
		//associative. Thus, x.y[4].z = ((x.y)[4]).z
		precedence_map["["]     = ++n;
		precedence_map["("]     = n;
		precedence_map["."]     = n;
	}
	
	assert(precedence_map.count(std::string(t.begin,t.end)));
	return precedence_map[std::string(t.begin,t.end)];
}

expression_ptr parse_expression(const variant& formula_str, const token* i1, const token* i2, function_symbol_table* symbols, const formula_callable_definition* callable_def, bool* can_optimize=NULL);

void parse_function_args(variant formula_str, const token* &i1, const token* i2,
						 std::vector<std::string>* res,
						 std::vector<std::string>* types,
						 std::vector<variant_type_ptr>* variant_types,
						 std::vector<variant>* default_values,
						 variant_type_ptr* result_type)
{
	if(i1->type == TOKEN_LPARENS) {
		++i1;
	} else {
		ASSERT_LOG(false, "Invalid function definition\n" << pinpoint_location(formula_str, i1->begin, (i2-1)->end));
	}
	
	while((i1->type != TOKEN_RPARENS) && (i1 != i2)) {
		variant_type_ptr variant_type_info;
		if(i1+1 != i2 && i1->type != TOKEN_COMMA && (i1+1)->type != TOKEN_COMMA && (i1+1)->type != TOKEN_RPARENS && std::string((i1+1)->begin, (i1+1)->end) != "=" && get_formula_callable_definition(std::string(i1->begin, i1->end)) == NULL) {
			variant_type_info = parse_variant_type(formula_str, i1, i2);
			std::string class_name;
			variant_type_info->is_class(&class_name);
		}

		ASSERT_LOG(i1->type != TOKEN_RPARENS && i1 != i2, "UNEXPECTED END OF FUNCTION DEF: " << pinpoint_location(formula_str, (i1-1)->begin, (i1-1)->end));

		if(i1->type == TOKEN_IDENTIFIER) {
			if(i1+1 != i2 && std::string((i1+1)->begin, (i1+1)->end) == "=") {
				types->push_back("");
				res->push_back(std::string(i1->begin, i1->end));
				variant_types->push_back(variant_type_info);

				i1 += 2;
				ASSERT_LOG(i1 != i2, "Invalid function definition\n" << pinpoint_location(formula_str, i1->begin, (i2-1)->end));

				const token* begin = i1;
				if(!token_matcher().add(TOKEN_COMMA).add(TOKEN_RPARENS)
				    .find_match(i1, i2)) {
					ASSERT_LOG(false, "Invalid function definition\n" << pinpoint_location(formula_str, i1->begin, (i2-1)->end));
				}

				const expression_ptr expr = parse_expression(
				    formula_str, begin, i1, NULL, NULL);

				boost::intrusive_ptr<map_formula_callable> callable(new map_formula_callable);
				default_values->push_back(expr->evaluate(*callable));
				if(variant_type_info && !variant_type_info->match(default_values->back())) {
					ASSERT_LOG(false, "Default argument to function doesn't match type for argument " << (types->size()+1) << " arg: " << default_values->back().write_json() << " AT: " << pinpoint_location(formula_str, i1->begin, (i2-1)->end));
				}

				continue;

			} else if(default_values->empty() == false) {
				ASSERT_LOG(i1 != i2, "Invalid function definition: some args do not have a default value after some args do\n" << pinpoint_location(formula_str, i1->begin, (i2-1)->end));
			} else if(i1+1 != i2 && std::string((i1+1)->begin, (i1+1)->end) == "*") {
				types->push_back("");
				res->push_back(std::string(i1->begin, i1->end) + std::string("*"));
				variant_types->push_back(variant_type_info);
				++i1;
			} else if(i1+1 != i2 && (i1+1)->type == TOKEN_IDENTIFIER) {
				types->push_back(std::string(i1->begin, i1->end));
				res->push_back(std::string((i1+1)->begin, (i1+1)->end));
				variant_types->push_back(variant_type_info);
				++i1;
			} else {
				types->push_back("");
				res->push_back(std::string(i1->begin, i1->end));
				variant_types->push_back(variant_type_info);
			}
		} else if (i1->type == TOKEN_COMMA) {
			//do nothing
		} else {
			ASSERT_LOG(false, "Invalid function definition\n" << pinpoint_location(formula_str, i1->begin, (i2-1)->end));
		}
		++i1;
	}
	
	if(i1->type != TOKEN_RPARENS) {
		ASSERT_LOG(false, "Invalid function definition\n" << pinpoint_location(formula_str, i1->begin, (i2-1)->end));
	}
	++i1;

	if(i1 != i2 && i1->type == TOKEN_POINTER) {
		++i1;
		ASSERT_LOG(i1 != i2, "Unexpected end of function definition: " << pinpoint_location(formula_str, (i1-1)->begin, (i1-1)->end));

		variant_type_ptr res = parse_variant_type(formula_str, i1, i2);
		if(result_type) {
			*result_type = res;
		}
	}
}

void parse_args(const variant& formula_str, const std::string* function_name,
                const token* i1, const token* i2,
				std::vector<expression_ptr>* res,
				function_symbol_table* symbols,
				const formula_callable_definition* definition,
				bool* can_optimize)
{
	std::vector<std::pair<const token*, const token*> > args;

	ASSERT_LE(i1, i2);
	int parens = 0;
	const token* beg = i1;
	while(i1 != i2) {
		if(i1->type == TOKEN_LPARENS || i1->type == TOKEN_LSQUARE || i1->type == TOKEN_LBRACKET ) {
			++parens;
		} else if(i1->type == TOKEN_RPARENS || i1->type == TOKEN_RSQUARE || i1->type == TOKEN_RBRACKET) {
			--parens;
		} else if(i1->type == TOKEN_COMMA && !parens) {
			args.push_back(std::pair<const token*, const token*>(beg, i1));
			beg = i1+1;
		}
		
		++i1;
	}
	
	if(beg != i1) {
		args.push_back(std::pair<const token*, const token*>(beg, i1));
	}

	for(int n = 0; n != args.size(); ++n) {
		const formula_callable_definition* callable_def = definition;

		if(n+1 == args.size()) {
			//Certain special functions take a special callable definition
			//to evaluate their last argument. Discover what that is here.
			static const std::string MapCallableFuncs[] = { "count", "filter", "find", "choose", "map", "count" };
			if(args.size() >= 2 && function_name != NULL && std::count(MapCallableFuncs, MapCallableFuncs + sizeof(MapCallableFuncs)/sizeof(*MapCallableFuncs), *function_name)) {
				std::string value_name = "value";

				static const std::string CustomIdMapCallableFuncs[] = { "filter", "find", "map" };
				if(args.size() == 3 && std::count(CustomIdMapCallableFuncs, CustomIdMapCallableFuncs + sizeof(CustomIdMapCallableFuncs)/sizeof(*CustomIdMapCallableFuncs), *function_name)) {
					//invocation like map(range(5), n, n*n) -- need to discover
					//the string for the second argument to set that in our
					//callable definition
					variant literal = res->back()->is_literal();
					if(literal.is_string()) {
						value_name = literal.as_string();
					} else if(res->back()->is_identifier(&value_name) == false) {
						ASSERT_LOG(false, "Function " << *function_name << " requires a literal as its second argument: " << pinpoint_location(formula_str, args[1].first->begin, (args[1].second-1)->end));
					}
				}
				ASSERT_LOG(args.size() == 2 || args.size() == 3, std::cerr << "WRONG NUMBER OF ARGS TO " << function_name << " AT " << pinpoint_location(formula_str, args[0].first->begin, (args[0].second-1)->end));

				variant_type_ptr key_type, value_type;

				variant_type_ptr sequence_type = (*res)[0]->query_variant_type();
				value_type = sequence_type->is_list_of();
				if(!value_type) {
					key_type = sequence_type->is_map_of().first;
					value_type = sequence_type->is_map_of().second;
				}

				callable_def = get_map_callable_definition(callable_def, key_type, value_type, value_name);
			}
		}

		if(function_name != NULL &&
		   (n == 1 && (*function_name == "sort" || *function_name == "fold") ||
		    n == 2 &&  *function_name == "zip")) {
			variant_type_ptr sequence_type = (*res)[0]->query_variant_type();
			variant_type_ptr value_type = sequence_type->is_list_of();
			callable_def = get_variant_comparator_definition(callable_def, value_type);
		}

		res->push_back(parse_expression(formula_str, args[n].first, args[n].second, symbols, callable_def, can_optimize));
	}
}

void parse_set_args(const variant& formula_str, const token* i1, const token* i2,
					std::vector<expression_ptr>* res,
					function_symbol_table* symbols,
				    const formula_callable_definition* callable_def)
{
	int parens = 0;
	bool check_pointer = false;
	const token* beg = i1;
	while(i1 != i2) {
		if(i1->type == TOKEN_LPARENS || i1->type == TOKEN_LSQUARE || i1->type == TOKEN_LBRACKET) {
			++parens;
		} else if(i1->type == TOKEN_RPARENS || i1->type == TOKEN_RSQUARE || i1->type == TOKEN_RBRACKET) {
			--parens;
		} else if((i1->type == TOKEN_POINTER || i1->type == TOKEN_COLON) && !parens ) {
			if (!check_pointer) {
				check_pointer = true;

				if(i1 - beg == 1 && beg->type == TOKEN_IDENTIFIER) {
					//make it so that {a: 4} is the same as {'a': 4}
					res->push_back(expression_ptr(new string_expression(std::string(beg->begin, beg->end), false, symbols)));
				} else {
					res->push_back(parse_expression(formula_str, beg,i1, symbols, callable_def));
				}
				beg = i1+1;
			} else {
				ASSERT_LOG(false, "Too many ':' operators.\n" << pinpoint_location(formula_str, i1->begin, (i2-1)->end));
			}
		} else if( i1->type == TOKEN_COMMA && !parens ) {
			check_pointer = false;
			res->push_back(parse_expression(formula_str, beg,i1, symbols, callable_def));
			beg = i1+1;
		}
		
		++i1;
	}
	
	if(beg != i1) {
		res->push_back(parse_expression(formula_str, beg,i1, symbols, callable_def));
	}
}

void parse_where_clauses(const variant& formula_str,
                         const token* i1, const token * i2,
						 expr_table_ptr res, function_symbol_table* symbols,
						 const formula_callable_definition* callable_def) {
	int parens = 0;
	const token *original_i1_cached = i1;
	const token *beg = i1;
	std::string var_name;
	while(i1 != i2) {
		if(i1->type == TOKEN_LPARENS || i1->type == TOKEN_LBRACKET || i1->type == TOKEN_LSQUARE) {
			++parens;
		} else if(i1->type == TOKEN_RPARENS || i1->type == TOKEN_RBRACKET || i1->type == TOKEN_RSQUARE) {
			--parens;
		} else if(!parens) {
			if(i1->type == TOKEN_COMMA) {
				if(var_name.empty()) {
					ASSERT_LOG(false, "There is 'where <expression>,; "
					<< "'where name=<expression>,' was needed.\n" <<
					pinpoint_location(formula_str, i1->begin));
				}
				(*res)[var_name] = parse_expression(formula_str, beg,i1, symbols, callable_def);
				beg = i1+1;
				var_name = "";
			} else if(i1->type == TOKEN_OPERATOR) {
				std::string op_name(i1->begin, i1->end);
				if(op_name == "=") {
					if(beg->type != TOKEN_IDENTIFIER || beg+1 != i1 || !var_name.empty()) {
						ASSERT_LOG(false, "Unexpected tokens after where\n"
						  << pinpoint_location(formula_str, i1->begin));
					}
					var_name.insert(var_name.end(), beg->begin, beg->end);
					beg = i1+1;
				}
			}
		}
		++i1;
	}
	if(beg != i1) {
		if(var_name.empty()) {
			ASSERT_LOG(false, "Unexpected tokens after where\n" <<
			            pinpoint_location(formula_str, beg->begin));
		}
		(*res)[var_name] = parse_expression(formula_str, beg,i1, symbols, callable_def);
	}
}

expression_ptr parse_expression_internal(const variant& formula_str, const token* i1, const token* i2, function_symbol_table* symbols, const formula_callable_definition* callable_def, bool* can_optimize=NULL);

namespace {
	
	//only allow one static_formula_callable to be active at a time.
	bool static_formula_callable_active = false;
	
	//a special callable which will throw an exception if it's actually called.
	//we use this to determine if an expression is static -- i.e. doesn't
	//depend on input, and can be reduced to its result.
	struct non_static_expression_exception {};
	class static_formula_callable : public formula_callable {
		static_formula_callable(const static_formula_callable&);
	public:
		static_formula_callable() : formula_callable(false) {
			if(static_formula_callable_active) {
				throw non_static_expression_exception();
			}
			
			static_formula_callable_active = true;
		}
		
		~static_formula_callable() {
			static_formula_callable_active = false;
		}
		
		variant get_value(const std::string& key) const {
			throw non_static_expression_exception();
		}

		variant get_value_by_slot(int slot) const {
			throw non_static_expression_exception();
		}
	};
}

int in_static_context = 0;
struct static_context {
	static_context() { ++in_static_context; }
	~static_context() { --in_static_context; }
};
		
expression_ptr optimize_expression(expression_ptr result, function_symbol_table* symbols, const formula_callable_definition* callable_def, bool reduce_to_static)
{
	expression_ptr original = result;

	if(g_strict_formula_checking) {
		original->perform_static_error_analysis();
	}

	if(reduce_to_static) {
		//we want to try to evaluate this expression, and see if it is static.
		//it is static if it never reads its input, if it doesn't call the rng,
		//and if a reference to the input itself is not stored.
		try {
			const unsigned int rng_seed = rng::get_seed();
			formula_callable_ptr static_callable(new static_formula_callable);

			variant res;
			
			{
				const static_context ctx;
				res = result->static_evaluate(*static_callable);
			}

			if(rng_seed == rng::get_seed() && static_callable->refcount() == 1) {
				//this expression is static. Reduce it to its result.
				result = expression_ptr(new variant_expression(res));
			}

			//it's possible if there is a latent reference to it the
			//static callable won't get destroyed, so make sure we
			//mark it as inactive to allow others to be created.
			static_formula_callable_active = false;
		} catch(non_static_expression_exception& e) {
			//the expression isn't static. Not an error.
		}
	}

	if(result) {
		expression_ptr optimized = result->optimize();
		if(optimized) {
			result = optimized;
		}
	}
	
	if(result) {
		result->copy_debug_info_from(*original);
	}

	return result;
}

expression_ptr parse_expression(const variant& formula_str, const token* i1, const token* i2, function_symbol_table* symbols, const formula_callable_definition* callable_def, bool* can_optimize)
{
	bool optimize = true;
	expression_ptr result(parse_expression_internal(formula_str, i1, i2, symbols, callable_def, &optimize));
	result->set_debug_info(formula_str, i1->begin, (i2-1)->end);

	result = optimize_expression(result, symbols, callable_def, optimize);

	if(can_optimize && !optimize) {
		*can_optimize = false;
	}

	return result;
}

//only returns a value in the case of a lambda function, otherwise
//returns NULL.
expression_ptr parse_function_def(const variant& formula_str, const token*& i1, const token* i2, function_symbol_table* symbols, const formula_callable_definition* callable_def)
{
	assert(i1->type == TOKEN_KEYWORD && std::string(i1->begin, i1->end) == "def");

	++i1;

	std::string formula_name;
	if(i1->type == TOKEN_IDENTIFIER) {
		formula_name = std::string(i1->begin, i1->end);
		++i1;
	}
	
	std::vector<std::string> args, types;
	std::vector<variant> default_args;
	std::vector<variant_type_ptr> variant_types;
	variant_type_ptr result_type;
	parse_function_args(formula_str, i1, i2, &args, &types, &variant_types, &default_args, &result_type);
	const token* beg = i1;
	while((i1 != i2) && (i1->type != TOKEN_SEMICOLON)) {
		++i1;
	}
	const std::string function_str = std::string(beg->begin, (i1-1)->end);
	variant function_var(function_str);
	if(formula_str.get_debug_info()) {
		//Set the debugging info for this new string, adjusting relative
		//to our parent formula, so we know where in the file it lies.
		const variant::debug_info* cur_info = formula_str.get_debug_info();
		variant::debug_info info = *cur_info;
		for(std::string::const_iterator i = formula_str.as_string().begin();
		    i != beg->begin; ++i) {
			if(*i == '\n') {
				info.line++;
				info.column = 0;
			} else {
				info.column++;
			}
		}

		function_var.set_debug_info(info);
	}
	
	recursive_function_symbol_table recursive_symbols(formula_name.empty() ? "recurse" : formula_name, args, default_args, symbols, formula_name.empty() ? callable_def : NULL, variant_types);

	//create a definition of the callable representing
	//function arguments.
	formula_callable_definition_ptr args_definition;
	const formula_callable_definition* args_definition_ptr = NULL;
	if(args.size()) {
		args_definition = create_formula_callable_definition(&args[0], &args[0] + args.size(), formula_name.empty() ? callable_def : NULL /*only get the surrounding scope if we have a lambda function.*/);
	} else if(formula_name.empty()) {
		//empty arg lambda function. Give the definition as our context.
		args_definition_ptr = callable_def;
	}

	if(args_definition) {
		args_definition_ptr = args_definition.get();
	}

	if(formula_name.empty() == false) {
		for(int n = 0; n != types.size(); ++n) {
			ASSERT_LOG(n < args.size(), "FORMULA ARGS MIS-MATCH");

			if(types[n].empty()) {
				continue;
			}

			ASSERT_LOG(args_definition->get_entry_by_id(args[n]) != NULL, "FORMULA FUNCTION TYPE ARGS MIS-MATCH\n" << pinpoint_location(formula_str, i1->begin, i1->end));

			const formula_callable_definition* def = get_formula_callable_definition(types[n]);
			ASSERT_LOG(def != NULL, "TYPE NOT FOUND: " << types[n] << "\n" << pinpoint_location(formula_str, i1->begin, i1->end));
			args_definition->get_entry_by_id(args[n])->type_definition = def;
		}
	}

	if(args_definition) {
		for(int n = 0; n != variant_types.size(); ++n) {
			args_definition->get_entry_by_id(args[n])->set_variant_type(variant_types[n]);
		}
	}

	const_formula_ptr fml(new formula(function_var, &recursive_symbols, args_definition_ptr));
	recursive_symbols.resolve_recursive_calls(fml);
	
	if(formula_name.empty()) {
		return expression_ptr(new lambda_function_expression(args, fml, callable_def ? callable_def->num_slots() : 0, default_args, variant_types, result_type ? result_type : fml->query_variant_type()));
	}

	const std::string precond = "";
	symbols->add_formula_function(formula_name, fml,
								  formula::create_optional_formula(variant(precond), symbols), args, default_args, variant_types);
	return expression_ptr();
}

expression_ptr parse_expression_internal(const variant& formula_str, const token* i1, const token* i2, function_symbol_table* symbols, const formula_callable_definition* callable_def, bool* can_optimize)
{
	ASSERT_LOG(i1 != i2, "Empty expression in formula\n" << pinpoint_location(formula_str, (i1-1)->end));
	
	if(symbols && i1->type == TOKEN_KEYWORD && std::string(i1->begin, i1->end) == "def" &&
	   ((i1+1)->type == TOKEN_IDENTIFIER || (i1+1)->type == TOKEN_LPARENS)) {

		expression_ptr lambda = parse_function_def(formula_str, i1, i2, symbols, callable_def);
		if(lambda) {
			return lambda;
		}

		if((i1 == i2) || (i1 == (i2-1))) {
			return expression_ptr(new function_list_expression(symbols));
		}
		else {
			return parse_expression(formula_str, (i1+1), i2, symbols, callable_def, can_optimize);
		}
	}
	
	int parens = 0;
	const token* op = NULL;
	const token* fn_call = NULL;
	
	for(const token* i = i1; i != i2; ++i) {
		if(fn_call && i+1 == i2 && i->type != TOKEN_RPARENS) {
			fn_call = NULL;
		}
		
		if(i->type == TOKEN_LPARENS || i->type == TOKEN_LSQUARE || i->type == TOKEN_LBRACKET) {
			if(i->type == TOKEN_LPARENS && parens == 0 && i != i1) {
				fn_call = i;
			} else if(i->type == TOKEN_LSQUARE && parens == 0 && i != i1 && (i-1)->type != TOKEN_OPERATOR && (op == NULL || operator_precedence(*op) >= operator_precedence(*i))) {
				//the square bracket itself is an operator
				op = i;
			}
			
			++parens;
		} else if(i->type == TOKEN_RPARENS || i->type == TOKEN_RSQUARE || i->type == TOKEN_RBRACKET) {
			--parens;
			
			if(parens == 0 && i+1 != i2) {
				fn_call = NULL;
			}
		} else if(parens == 0 && (i->type == TOKEN_OPERATOR || i->type == TOKEN_LEFT_POINTER)) {
			if(op == NULL || operator_precedence(*op) >= operator_precedence(*i)) {
				if(i != i1 && i->end - i->begin == 3 && std::equal(i->begin, i->end, "not")) {
					//The not operator is always unary and can only
					//appear at the start of an expression.
					continue;
				}

				op = i;
			}
		}
	}
	
	if(op != NULL && op->type == TOKEN_LSQUARE) {
		//the square bracket operator is handled below, just set the op
		//to NULL and it'll be handled.
		op = NULL;
	}
	
	if(op == NULL) {
		if(i1->type == TOKEN_LPARENS && (i2-1)->type == TOKEN_RPARENS) {
			return parse_expression(formula_str, i1+1,i2-1,symbols, callable_def, can_optimize);
		} else if( (i2-1)->type == TOKEN_RSQUARE) { //check if there is [ ] : either a list definition, or a operator 
			const token* tok = i2-2;
			int square_parens = 0;
			while ( (tok->type != TOKEN_LSQUARE || square_parens) && tok != i1) {
				if (tok->type == TOKEN_RSQUARE) {
					square_parens++;
				} else if(tok->type == TOKEN_LSQUARE) {
					square_parens--;
				}
				--tok;
			}	

			if (tok->type == TOKEN_LSQUARE) {
				if (tok == i1) {
					const token* pipe = i1+1;
					if(token_matcher().add(TOKEN_PIPE).find_match(pipe, i2)) {
						//a list comprehension
						const token* const begin_start_expr = i1+1;

						typedef std::pair<const token*,const token*> Arg;
						std::vector<Arg> args;
						const token* arg = pipe+1;
						const token* end_arg = arg;
						while(token_matcher().add(TOKEN_COMMA).find_match(end_arg, i2-1)) {
							args.push_back(Arg(arg, end_arg));
							arg = ++end_arg;
						}
						args.push_back(Arg(arg, i2-1));

						std::map<std::string, expression_ptr> generators;
						std::vector<expression_ptr> filter_expr;

						std::vector<std::string> items;
						std::map<std::string, variant_type_ptr> item_types;

						const_formula_callable_definition_ptr def;

						bool seen_filter = false;

						foreach(const Arg& arg, args) {
							const token* arrow = arg.first;
							if(token_matcher().add(TOKEN_LEFT_POINTER).find_match(arrow, arg.second)) {
								ASSERT_LOG(arrow - arg.first == 1 && arg.first->type == TOKEN_IDENTIFIER, "expected identifier to the left of <- in list comprehension\n" << pinpoint_location(formula_str, arg.first->begin, arrow->end));
								ASSERT_LOG(!seen_filter, "found <- after finding a filter in list comprehension\n" << pinpoint_location(formula_str, arg.first->begin, arrow->end));

								const std::string key(arg.first->begin, arg.first->end);
								ASSERT_LOG(generators.count(key) == 0, "repeated identifier in list generator: " << key << "\n" << pinpoint_location(formula_str, arg.first->begin, arrow->end));

								generators[key] = parse_expression(formula_str, arrow+1, arg.second, symbols, callable_def, can_optimize);
								items.push_back(key);
								variant_type_ptr gen_type = generators[key]->query_variant_type();

								if(gen_type) {
									gen_type = gen_type->is_list_of();
								}

								if(!gen_type) {
									gen_type = variant_type::get_any();
								}

								item_types[key] = gen_type;
							} else {
								if(!def) {
									ASSERT_LOG(items.empty() == false, "EMPTY ITEMS IN LIST COMPREHENSION: " << pinpoint_location(formula_str, arrow->begin, arrow->end));
									std::sort(items.begin(), items.end());
									std::vector<variant_type_ptr> types;
									foreach(const std::string& item, items) {
										types.push_back(item_types[item]);
									}
									def = create_formula_callable_definition(&items[0], &items[0] + items.size(), callable_def, &types[0]);
								}
								filter_expr.push_back(parse_expression(formula_str, arg.first, arg.second, symbols, def.get(), can_optimize));
								seen_filter = true;
							}
						}

						if(!def) {
							ASSERT_LOG(items.empty() == false, "EMPTY ITEMS IN LIST COMPREHENSION: " << pinpoint_location(formula_str, pipe->begin, pipe->end));
							std::sort(items.begin(), items.end());
							std::vector<variant_type_ptr> types;
							foreach(const std::string& item, items) {
								types.push_back(item_types[item]);
							}
							def = create_formula_callable_definition(&items[0], &items[0] + items.size(), callable_def, &types[0]);
						}

						expression_ptr expr = parse_expression(formula_str, begin_start_expr, pipe, symbols, def.get(), can_optimize);

						return expression_ptr(new list_comprehension_expression(expr, generators, filter_expr, callable_def ? callable_def->num_slots() : 0));
					} else {
						//create a list
						std::vector<expression_ptr> args;
						parse_args(formula_str,NULL,i1+1,i2-1,&args,symbols, callable_def, can_optimize);
						return expression_ptr(new list_expression(args));
					}
				} else {
					//determine if it's an array-style access of a single list element, or a slice.
					const token* tok2 = i2-2;
					int bracket_parens_count = 0;
					const token* colon_tok = NULL;
					while (tok2 != tok){
						if (tok2->type == TOKEN_RSQUARE || tok2->type == TOKEN_RPARENS) {
							bracket_parens_count++;
						} else if (tok2->type == TOKEN_LSQUARE || tok2->type == TOKEN_LPARENS){
							bracket_parens_count--;
						} else if (tok2->type == TOKEN_COLON){
							if(bracket_parens_count != 0){
									//TODO - handle error - mismatching brackets
									std::cerr << "mismatching brackets or parentheses inside [ ]: '" << std::string((i1+1)->begin, (i2-1)->end) << "'\n";
							} else if (colon_tok != NULL){
									//TODO - handle error - more than one colon.
									std::cerr << "more than one colon inside a slice [:]: '" << std::string((i1+1)->begin, (i2-1)->end) << "'\n";
							} else {
								colon_tok = tok2;
							}
						}
						--tok2;	
					}
					
					if(colon_tok != NULL){
						expression_ptr start, end;
						if(tok+1 < colon_tok) {
							start = parse_expression(formula_str, tok+1, colon_tok, symbols, callable_def, can_optimize);
						}

						if(colon_tok+1 < i2-1) {
							end = parse_expression(formula_str, colon_tok+1, i2-1, symbols, callable_def, can_optimize);
						}

						//it's a slice.  execute operator [ : ]
						return expression_ptr(new slice_square_bracket_expression(
																			parse_expression(formula_str, i1,tok,symbols, callable_def, can_optimize), start, end));
					}else{	
						//execute operator [ ]
						return expression_ptr(new square_bracket_expression(
																			parse_expression(formula_str, i1,tok,symbols, callable_def, can_optimize),
																			parse_expression(formula_str, tok+1,i2-1,symbols, callable_def, can_optimize)));
					}
				}
			}
		} else if(i1->type == TOKEN_LBRACKET && (i2-1)->type == TOKEN_RBRACKET) {
			//create a map TODO: add support for a set
			std::vector<expression_ptr> args;
			parse_set_args(formula_str,i1+1,i2-1,&args,symbols,callable_def);
			return expression_ptr(new map_expression(args));
		} else if(i2 - i1 == 1) {
			if(i1->type == TOKEN_KEYWORD) {
				if(std::string(i1->begin,i1->end) == "functions") {
					return expression_ptr(new function_list_expression(symbols));
				} else if(std::string(i1->begin,i1->end) == "null") {
					return expression_ptr(new null_expression());
				} else if(std::string(i1->begin,i1->end) == "true") {
					return expression_ptr(new variant_expression(variant::from_bool(true)));
				} else if(std::string(i1->begin,i1->end) == "false") {
					return expression_ptr(new variant_expression(variant::from_bool(false)));
				}
			} else if(i1->type == TOKEN_CONST_IDENTIFIER) {
				return expression_ptr(new const_identifier_expression(
																	  std::string(i1->begin,i1->end)));
			} else if(i1->type == TOKEN_IDENTIFIER) {
				if(can_optimize) {
					*can_optimize = false;
				}

				std::string symbol(i1->begin, i1->end);
				identifier_expression* expr =
				    new identifier_expression(symbol, callable_def);
				const formula_function* fn = symbols ? symbols->get_formula_function(symbol) : NULL;
				if(fn != NULL) {
					expression_ptr function(new lambda_function_expression(fn->args(), fn->get_formula(), 0, fn->default_args(), fn->variant_types(), variant_type::get_any()));
					expr->set_function(function);
				}
				return expression_ptr(expr);
			} else if(i1->type == TOKEN_INTEGER) {
				int n = strtol(std::string(i1->begin,i1->end).c_str(), NULL, 0);
				return expression_ptr(new integer_expression(n));
			} else if(i1->type == TOKEN_DECIMAL) {
				std::string decimal_string(i1->begin, i1->end);
				return expression_ptr(new decimal_expression(decimal::from_string(decimal_string)));
			} else if(i1->type == TOKEN_STRING_LITERAL) {
				bool translate = *(i1->begin) == '~';
				int add = *(i1->begin) == 'q' ? 2 : 1;
				return expression_ptr(new string_expression(std::string(i1->begin+add,i1->end-1), translate, symbols));
			}
		} else if(i1->type == TOKEN_IDENTIFIER &&
				  (i1+1)->type == TOKEN_LPARENS &&
				  (i2-1)->type == TOKEN_RPARENS) {
			int nleft = 0, nright = 0;
			for(const token* i = i1; i != i2; ++i) {
				if(i->type == TOKEN_LPARENS) {
					++nleft;
				} else if(i->type == TOKEN_RPARENS) {
					++nright;
				}
			}
			
			if(nleft == nright) {
				const std::string function_name(i1->begin, i1->end);
				std::vector<expression_ptr> args;
				parse_args(formula_str,&function_name,i1+2,i2-1,&args,symbols, callable_def, can_optimize);
				expression_ptr result(create_function(function_name, args, symbols, callable_def));
				if(result) {
					return result;
				}
			}
		}
		
		if(!fn_call) {
			if(i1->type == TOKEN_IDENTIFIER && (i1+1)->type == TOKEN_LPARENS) {
				const token* match = i1+2;
				int depth = 0;
				while(match < i2) {
					if(match->type == TOKEN_LPARENS) {
						++depth;
					} else if(match->type == TOKEN_RPARENS) {
						if(depth == 0) {
							break;
						}
						--depth;
					}
					++match;
				}

				if(match != i2) {
					++match;
					ASSERT_LT(match, i2); 

					ASSERT_LOG(false, "unexpected tokens after function call\n" << pinpoint_location(formula_str, match->begin, (i2-1)->end));
				} else {
					ASSERT_LOG(false, "no closing parenthesis to function call\n" << pinpoint_location(formula_str, i1->begin, (i2-1)->end));
				}
			} else {
				ASSERT_LOG(false, "could not parse expression\n" << pinpoint_location(formula_str, i1->begin, (i2-1)->end));
			}

			assert(false); //should never reach here.
		}
	}
	
	if(fn_call && (op == NULL ||
	   operator_precedence(*op) >= operator_precedence(*fn_call))) {
		op = fn_call;
	}
	
	if(op == i1) {
		if(op+1 == i2) {
			std::cerr << "No expression for operator '" << std::string(op->begin,op->end) << "' to operate on\n";
		}
		return expression_ptr(new unary_operator_expression(
															std::string(op->begin,op->end),
															parse_expression(formula_str, op+1,i2,symbols, callable_def, can_optimize)));
	}
	
	int consume_backwards = 0;
	std::string op_name(op->begin,op->end);

	if(op_name == "in" && op > i1 && op-1 > i1 && std::string((op-1)->begin, (op-1)->end) == "not") {
		op_name = "not in";
		consume_backwards = 1;
	}

	if(op_name == "<-") {
		variant_type_ptr type = parse_variant_type(formula_str, i1, op);
		ASSERT_LOG(type && i1 == op, "UNEXPECTED TOKENS WHEN PARSING TYPE: " << pinpoint_location(formula_str, i1->begin, op->end));

		expression_ptr right(parse_expression(formula_str, op+1,i2,symbols, callable_def, can_optimize));

		return expression_ptr(new type_expression(type, right));
	}
	
	if(op_name == "(") {
		if(i2 - op < 2) {
			ASSERT_LOG(false, "MISSING PARENS IN FORMULA\n" << pinpoint_location(formula_str, op->begin, op->end));
		}

		std::vector<expression_ptr> args;
		parse_args(formula_str,NULL,op+1, i2-1, &args, symbols, callable_def, can_optimize);
		
		return expression_ptr(new function_call_expression(
														   parse_expression(formula_str, i1, op, symbols, callable_def, can_optimize), args));
	}
	
	if(op_name == ".") {
		expression_ptr left(parse_expression(formula_str, i1,op,symbols, callable_def, can_optimize));
		const formula_callable_definition* type_definition = left->get_type_definition();
		return expression_ptr(new dot_expression(left,
												 parse_expression(formula_str, op+1,i2,NULL, type_definition, can_optimize)));
	}
	
	if(op_name == "where") {
		const int base_slots = callable_def ? callable_def->num_slots() : 0;
		where_variables_info_ptr where_info(new where_variables_info(base_slots));

		expr_table_ptr table(new expr_table());
		parse_where_clauses(formula_str, op+1, i2, table, symbols, callable_def);
		std::vector<expression_ptr> entries;
		for(expr_table::iterator i = table->begin(); i != table->end(); ++i) {
			where_info->names.push_back(i->first);
			where_info->entries.push_back(i->second);
		}

		const_formula_callable_definition_ptr callable_where_def = create_where_definition(table, callable_def);
		return expression_ptr(new where_expression(parse_expression(formula_str, i1, op, symbols, callable_where_def.get(), can_optimize), where_info));
	} else if(op_name == "asserting") {
		expression_ptr debug_expr;

		const token* pipe = op+1;
		if(token_matcher().add(TOKEN_PIPE).find_match(pipe, i2)) {
			debug_expr = parse_expression(formula_str, pipe+1, i2, symbols, callable_def, can_optimize);
			i2 = pipe;
		}

		expression_ptr base_expr(parse_expression(formula_str, i1, op, symbols, callable_def, can_optimize));
		std::vector<expression_ptr> asserts;
		parse_args(formula_str,NULL,op+1,i2,&asserts,symbols, callable_def, can_optimize);

		return expression_ptr(new assert_expression(base_expr, asserts, debug_expr));
	}

	const bool is_dot = op_name == ".";
	return expression_ptr(new operator_expression(
												  op_name, parse_expression(formula_str, i1,op-consume_backwards,symbols, callable_def, can_optimize),
												  parse_expression(formula_str, op+1,i2,symbols, callable_def, can_optimize)));
}
}

void formula::fail_if_static_context()
{
	if(in_static_context) {
		throw non_static_expression_exception();
	}
}

formula::strict_check_scope::strict_check_scope()
{
	++g_strict_formula_checking;
}

formula::strict_check_scope::~strict_check_scope()
{
	--g_strict_formula_checking;
}

formula_ptr formula::create_optional_formula(const variant& val, function_symbol_table* symbols, const formula_callable_definition* callable_definition)
{
	if(val.is_null() || val.is_string() && val.as_string().empty()) {
		return formula_ptr();
	}
	
	return formula_ptr(new formula(val, symbols, callable_definition));
}

formula::formula(const variant& val, function_symbol_table* symbols, const formula_callable_definition* callable_definition) : str_(val)
{
	using namespace formula_tokenizer;

	function_symbol_table symbol_table;
	if(!symbols) {
		symbols = &symbol_table;
	}

	if(str_.is_int() || str_.is_bool() || str_.is_decimal()) {
		//Allow ints, bools, and decimals to be interpreted as formulae.
		str_ = variant(str_.string_cast());
	}

	std::vector<token> tokens;
	std::string::const_iterator i1 = str_.as_string().begin(), i2 = str_.as_string().end();
	while(i1 != i2) {
		try {
			tokens.push_back(get_token(i1,i2));
			if((tokens.back().type == TOKEN_WHITESPACE) || (tokens.back().type == TOKEN_COMMENT)) {
				tokens.pop_back();
			}
		} catch(token_error& e) {
			ASSERT_LOG(false, "Token error: " << e.msg << ": " << pinpoint_location(str_, i1, i1));
		}
	}

	check_brackets_match(tokens);

	if(tokens.size() != 0) {
		const_formula_callable_definition_ptr global_where_def;

		const token* tok = &tokens[0];
		const token* end_tokens = &tokens[0] + tokens.size();

		if(tokens[0].type == TOKEN_KEYWORD && std::string(tokens[0].begin, tokens[0].end) == "base") {

			const token* recursive_case = tok;
			if(!token_matcher(TOKEN_KEYWORD).add("recursive").find_match(recursive_case, end_tokens)) {
				ASSERT_LOG(false, "ERROR WHILE PARSING FORMULA: NO RECURSIVE CASE FOUND");
			}


			const token* where_tok = recursive_case;

			if(token_matcher(TOKEN_OPERATOR).add("where").find_match(where_tok, end_tokens)) {
				global_where_.reset(new where_variables_info(callable_definition ? callable_definition->num_slots() : 0));
				expr_table_ptr table(new expr_table());
				parse_where_clauses(str_, where_tok+1, end_tokens, table, symbols, callable_definition);
				for(expr_table::iterator i = table->begin(); i != table->end(); ++i) {
					global_where_->names.push_back(i->first);
					global_where_->entries.push_back(i->second);
				}

				global_where_def = create_where_definition(table, callable_definition);
				callable_definition = global_where_def.get();

				end_tokens = where_tok;
			}

			while(tok->type == TOKEN_KEYWORD && std::string(tok->begin, tok->end) == "base") {
				++tok;

				const token* colon_ptr = tok;

				if(!token_matcher(TOKEN_COLON).find_match(colon_ptr, end_tokens)) {
					ASSERT_LOG(false, "ERROR WHILE PARSING FORMULA: ':' EXPECTED AFTER BASE");
				}

				const token* end_ptr = colon_ptr;

				if(!token_matcher(TOKEN_KEYWORD).add("base").add("recursive").find_match(end_ptr, end_tokens)) {
					ASSERT_LOG(false, "ERROR WHILE PARSING FORMULA: NO RECURSIVE CASE FOUND");
				}

				BaseCase base;
				base.raw_guard = base.guard = parse_expression(str_, tok, colon_ptr, symbols, callable_definition);
				base.expr = parse_expression(str_, colon_ptr+1, end_ptr, symbols, callable_definition);

				base_expr_.push_back(base);

				tok = end_ptr;
			}

			//check that the part before the actual formula is recursive:
			ASSERT_LOG(tok + 2 < end_tokens && tok->type == TOKEN_KEYWORD && std::string(tok->begin, tok->end) == "recursive" && (tok+1)->type == TOKEN_COLON, "RECURSIVE CASE NOT FOUND");

			tok += 2;

		}

		expr_ = parse_expression(str_, tok, end_tokens, symbols, callable_definition);

		if(global_where_) {
			expr_.reset(new where_expression(expr_, global_where_));
			foreach(BaseCase& base, base_expr_) {
				base.guard.reset(new where_expression(base.guard, global_where_));
				base.expr.reset(new where_expression(base.expr, global_where_));
			}
		}
	} else {
		expr_ = expression_ptr(new null_expression());
	}	
}

const_formula_callable_ptr formula::wrap_callable_with_global_where(const formula_callable& callable) const
{
	if(global_where_) {
		const_formula_callable_ptr wrapped_variables(new where_variables(callable, global_where_));
		return wrapped_variables;
	} else {
		return const_formula_callable_ptr(&callable);
	}
}

variant_type_ptr formula::query_variant_type() const
{
	return expr_->query_variant_type();
}

void formula::check_brackets_match(const std::vector<token>& tokens) const
{
	std::string error_msg;
	int error_loc = -1;

	std::stack<formula_tokenizer::FFL_TOKEN_TYPE> brackets;
	std::stack<int> brackets_locs;
	for(int n = 0; n != tokens.size(); ++n) {
		switch(tokens[n].type) {
		case TOKEN_LPARENS:
		case TOKEN_LSQUARE:
		case TOKEN_LBRACKET:
			brackets.push(tokens[n].type);
			brackets_locs.push(n);
			break;
		case TOKEN_RPARENS:
		case TOKEN_RSQUARE:
		case TOKEN_RBRACKET:
			if(brackets.empty()) {
				error_msg = "UNEXPECTED TOKEN: " + std::string(tokens[n].begin, tokens[n].end);
				error_loc = n;
				break;
			} else if(brackets.top() != tokens[n].type-1) {
				const int m = brackets_locs.top();
				error_msg = "UNMATCHED BRACKET: " + std::string(tokens[m].begin, tokens[m].end);
				error_loc = m;
				break;
			}

			brackets.pop();
			brackets_locs.pop();
			break;
		default:
			break;
		}
	}

	if(error_msg.empty() && brackets.empty() == false) {
		const int m = brackets_locs.top();
		error_msg = "UNMATCHED BRACKET: " + std::string(tokens[m].begin, tokens[m].end);
		error_loc = m;
	}

	if(error_loc != -1) {
		const token& tok = tokens[error_loc];
		std::string::const_iterator begin_line = tokens.front().begin;
		std::string::const_iterator i = begin_line;
		int nline = 0;
		while(i < tok.begin) {
			if(i == tok.begin) {
				break;
			}

			if(*i == '\n') {
				++nline;
				begin_line = i+1;
			}
			++i;
		}

		const std::string::const_iterator end_line = std::find(begin_line, tokens.back().end, '\n');
		while(begin_line < end_line && util::c_isspace(*begin_line)) {
			++begin_line;
		}

		std::string whitespace(begin_line, tok.begin);
		std::fill(whitespace.begin(), whitespace.end(), ' ');
		std::string error_line(begin_line, end_line);

		if(whitespace.size() > 60) {
			const int erase_size = whitespace.size() - 60;
			whitespace.erase(whitespace.begin(), whitespace.begin() + erase_size);
			ASSERT_LOG(erase_size <= error_line.size(), "ERROR WHILE PARSING ERROR MESSAGE: " << erase_size << " <= " << error_line.size() << " IN " << error_line);
			error_line.erase(error_line.begin(), error_line.begin() + erase_size);
			std::fill(error_line.begin(), error_line.begin() + 3, '.');
		}

		if(error_line.size() > 78) {
			error_line.resize(78);
			std::fill(error_line.end()-3, error_line.end(), '.');
		}
		

		std::string location;
		const variant::debug_info* dbg_info = str_.get_debug_info();
		if(dbg_info) {
			location = formatter() << " AT " << *dbg_info->filename
		                           << " " << dbg_info->line;
		}
		//TODO: extract info from str_ about the location of the formula.
		ASSERT_LOG(false, "ERROR WHILE PARSING FORMULA" << location << ": "
		  << error_msg << "\n"
		  << error_line << "\n"
		  << whitespace << "^\n");
	}
}


formula::~formula() {
	if(last_executed_formula == this) {
		last_executed_formula = NULL;
	}
}

std::string formula::output_debug_info() const
{
	std::ostringstream s;
	s << "FORMULA: " << (str_.get_debug_info() ? str_.get_debug_info()->message() : "(UNKNOWN LOCATION): ");
	//TODO: add debug info from str_ variant here.
	
	s << str_.as_string() << "\n";
	return s.str();
}

int formula::guard_matches(const formula_callable& variables) const
{
	if(base_expr_.empty() == false) {
		int index = 0;
		foreach(const BaseCase& b, base_expr_) {
			if(b.guard->evaluate(variables).as_bool()) {
				return index;
			}

			++index;
		}
	}

	return -1;
}

int formula::raw_guard_matches(const formula_callable& variables) const
{
	if(base_expr_.empty() == false) {
		int index = 0;
		foreach(const BaseCase& b, base_expr_) {
			if(b.raw_guard->evaluate(variables).as_bool()) {
				return index;
			}

			++index;
		}
	}

	return -1;
}

variant formula::execute(const formula_callable& variables) const
{
	//We want to track the 'last executed' formula in last_executed_formula,
	//so we can use it for debugging purposes if there's a problem.
	//If one formula calls another, we want to restore the old value after
	//the nested formula exits. However, when a formula returns, if it's
	//the top-level formula we want to still keep it recorded as the
	//last executed, so we can complain about it if any commands it returns
	//have problems.
	//
	//As such we track the depth of the execution stack so we can tell if
	//we're a top-level formula or not. If we're a nested formula we restore
	//last_executed_formula upon return.
	//
	//Naturally if we throw an exception we DON'T want to restore the
	//last_executed_formula since we want to report the error.
	static int execution_stack = 0;
	const formula* prev_executed = execution_stack ? last_executed_formula : NULL;
	last_executed_formula = this;
	try {
		++execution_stack;

		const int nguard = guard_matches(variables);

		variant result = (nguard == -1 ? expr_ : base_expr_[nguard].expr)->evaluate(variables);
		--execution_stack;
		if(prev_executed) {
			last_executed_formula = prev_executed;
		}
		return result;
	} catch(std::string& e) {
//		output_debug_info();
//		ASSERT_LOG(false, "formula type error: " << e.message << "\n");
	}

	ASSERT_LOG(false, "");
}

variant formula::execute() const
{
	last_executed_formula = this;
	
	map_formula_callable* null_callable = new map_formula_callable;
	variant ref(null_callable);
	return execute(*null_callable);
}

bool formula::evaluates_to_constant(variant& result) const
{
	return expr_->can_reduce_to_variant(result);
}

UNIT_TEST(recursive_call_lambda) {
	CHECK(formula(variant("def fact_tail(n,a,b) factt(n,1) where factt = def(m,x) if(m > 0, x + m + recurse(m-1,x*m),x); fact_tail(5,0,0)")).execute() != variant(), "test failed");
}

UNIT_TEST(formula_slice) {
	CHECK(formula(variant("myList[2:4] where myList = [1,2,3,4,5,6]")).execute() == formula(variant("[3,4]")).execute(), "test failed");
	CHECK(formula(variant("myList[0:2] where myList = [1,2,3,4,5,6]")).execute() == formula(variant("[1,2]")).execute(), "test failed");
	CHECK(formula(variant("myList[1:4] where myList = [0,2,4,6,8,10,12,14]")).execute() == formula(variant("[2,4,6]")).execute(), "test failed");
}
	
	
UNIT_TEST(formula_in) {
	CHECK(formula(variant("1 in [4,5,6]")).execute() == variant::from_bool(false), "test failed");
	CHECK(formula(variant("5 in [4,5,6]")).execute() == variant::from_bool(true), "test failed");
	CHECK(formula(variant("5 not in [4,5,6]")).execute() == variant::from_bool(false), "test failed");
	CHECK(formula(variant("8 not in [4,5,6]")).execute() == variant::from_bool(true), "test failed");
}

UNIT_TEST(formula_fn) {
	function_symbol_table symbols;
	CHECK(formula(variant("def f(g) g(5) + 1; def fn(n) n*n; f(fn)"), &symbols).execute() == variant(26), "test failed");
}

UNIT_TEST(array_index) {
	formula f(variant("map(range(6), 'n', elements[n]) = elements "
			          "where elements = [5, 6, 7, 8, 9, 10]"));
	CHECK(f.execute() == variant::from_bool(true), "test failed");
}

UNIT_TEST(dot_precedence) {
	map_formula_callable* callable = new map_formula_callable;
	variant ref(callable);
	map_formula_callable* callable2 = new map_formula_callable;
	std::vector<variant> v;
	for(int n = 0; n != 10; ++n) {
		map_formula_callable* obj = new map_formula_callable;
		obj->add("value", variant(n));
		v.push_back(variant(obj));
	}
	callable2->add("item", variant(&v));
	callable->add("obj", variant(callable2));
	formula f(variant("obj.item[n].value where n = 2"));
	const variant result = f.execute(*callable);
	CHECK(result == variant(2), "test failed: " << result.to_debug_string());
}

UNIT_TEST(short_circuit) {
	map_formula_callable* callable = new map_formula_callable;
	variant ref(callable);
	callable->add("x", variant(0));
	formula f(variant("x and (5/x)"));
	f.execute(*callable);
}

UNIT_TEST(formula_decimal) {
	CHECK_EQ(formula(variant("0.0005")).execute().string_cast(), "0.0005");
    CHECK_EQ(formula(variant("0.005")).execute().string_cast(), "0.005");
	CHECK_EQ(formula(variant("0.05")).execute().string_cast(), "0.05");
	CHECK_EQ(formula(variant("0.5")).execute().string_cast(), "0.5");
	CHECK_EQ(formula(variant("8.5 + 0.5")).execute().string_cast(), "9.0");
	CHECK_EQ(formula(variant("4 * (-1.1)")).execute().string_cast(), "-4.4");
}

UNIT_TEST(formula_quotes) {
	CHECK_EQ(formula(variant("q((4+2())) + q^a^")).execute().string_cast(), "(4+2())a");
}

UNIT_TEST(map_to_maps_FAILS) {
	CHECK_EQ(formula(variant("{'a' -> ({'b' -> 2})}")).execute().string_cast(), formula(variant("{'a' -> {'b' -> 2}}")).execute().string_cast());
}

UNIT_TEST(formula_test_recursion) {
	function_symbol_table symbols;
	formula f(variant(
"def silly_add(a, c)"
"base b <= 0: a "
"recursive: silly_add(a+1, b-1) where b = c;"
"silly_add(50, 5000)"), &symbols);

	CHECK_EQ(f.execute().as_int(), 5050);
}

UNIT_TEST(formula_test_recurse_sort) {
	formula f(variant(
"def my_qsort(items) "
"base size(items) <= 1: items "
"recursive: my_qsort(filter(items, i, i < items[0])) +"
"           filter(items, i, i = items[0]) +"
"           my_qsort(filter(items, i, i > items[0]));"
"my_qsort([4,10,2,9,1])"));
	CHECK_EQ(f.execute(), formula(variant("[1,2,4,9,10]")).execute());
}

UNIT_TEST(formula_where_map) {
	CHECK_EQ(formula(variant("{'a': a} where a = 4")).execute()["a"], variant(4));
}

UNIT_TEST(formula_function_default_args) {
	CHECK_EQ(formula(variant("def f(x=5) x ; f() + f(1)")).execute(), variant(6));
	CHECK_EQ(formula(variant("f(5) where f = def(x,y=2) x*y")).execute(), variant(10));
}

UNIT_TEST(formula_typeof) {
#define TYPEOF_TEST(a, b) CHECK_EQ(formula(variant(a)).execute(), variant(b))
	TYPEOF_TEST("typeof(def(int n) n+5)", "function(int) -> int");
	TYPEOF_TEST("typeof(def(int n) n+5.0)", "function(int) -> decimal");
	TYPEOF_TEST("typeof(def([int] mylist) map(mylist, value+5.0))", "function([int]) -> [decimal]");
	TYPEOF_TEST("typeof(choose([1,2,3]))", "int");
	TYPEOF_TEST("typeof(choose([1,2,'abc',4.5]))", "int|string|decimal");
	TYPEOF_TEST("typeof(if(1d6 = 5, 5))", "int|null");
	TYPEOF_TEST("typeof(if(1d6 = 2, 5, 8))", "int");
	TYPEOF_TEST("typeof(if(1d6 = 2, 'abc', 2))", "string|int");
#undef TYPEOF_TEST
}

UNIT_TEST(formula_list_comprehension) {
	std::vector<variant> result;
	for(int n = 0; n != 4; ++n) {
		result.push_back(variant(n));
	}

	CHECK_EQ(formula(variant("[x | x <- [0,1,2,3]]")).execute(), variant(&result));
	CHECK_EQ(formula(variant("[x | x <- [0,1,2,3], x%2 = 1]")).execute(), formula(variant("[1,3]")).execute());
}

BENCHMARK(formula_list_comprehension_bench) {
	formula f(variant("[x*x + 5 | x <- range(input)]"));
	static map_formula_callable* callable = new map_formula_callable;
	callable->add("input", variant(1000));
	BENCHMARK_LOOP {
		f.execute(*callable);
	}
}

BENCHMARK(formula_map_bench) {
	formula f(variant("map(range(input), value*value + 5)"));
	static map_formula_callable* callable = new map_formula_callable;
	callable->add("input", variant(1000));
	BENCHMARK_LOOP {
		f.execute(*callable);
	}
}

BENCHMARK(formula_recurse_sort) {
	formula f(variant(
"def my_qsort(items) if(size(items) <= 1, items,"
" my_qsort(filter(items, i, i < items[0])) +"
"          filter(items, i, i = items[0]) +"
" my_qsort(filter(items, i, i > items[0])));"
"my_qsort(input)"));

	std::vector<variant> input;
	for(int n = 0; n != 100000; ++n) {
		input.push_back(variant(n));
	}

	std::vector<variant> expected_result = input;
	variant expected_result_v(&expected_result);

	std::random_shuffle(input.begin(), input.end());
	static map_formula_callable* callable = new map_formula_callable;
	callable->add("input", variant(&input));
	BENCHMARK_LOOP {
		CHECK_EQ(f.execute(*callable), expected_result_v);
	}
}

BENCHMARK(formula_recursion) {
	formula f(variant(
"def my_index(ls, item, n)"
"base ls = []: -1 "
"base ls[0] = item: n "
"recursive: my_index(ls[1:], item, n+1);"
"my_index(range(1000001), pos, 0)"));

	formula f2(variant(
"def silly_add(a, b)"
"base b <= 0: a "
"recursive: silly_add(a+1, b-1);"
"silly_add(0, pos)"));
	static map_formula_callable* callable = new map_formula_callable;
	callable->add("pos", variant(100000));
	BENCHMARK_LOOP {
		CHECK_EQ(f.execute(*callable), variant(100000));
	}
}

BENCHMARK(formula_if) {
	static map_formula_callable* callable = new map_formula_callable;
	callable->add("x", variant(1));
	static formula f(variant("if(x, 1, 0)"));
	BENCHMARK_LOOP {
		f.execute(*callable);
	}
}

BENCHMARK(formula_add) {
	static map_formula_callable* callable = new map_formula_callable;
	callable->add("x", variant(1));
	static formula f(variant("x+1"));
	BENCHMARK_LOOP {
		f.execute(*callable);
	}
}

}
