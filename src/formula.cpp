/* $Id: formula.cpp 25895 2008-04-17 18:57:13Z mordante $ */
/*
 Copyright (C) 2007 by David White <dave.net>
 Part of the Silver Tree Project
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 or later.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY.
 
 See the COPYING file for more details.
 */
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <stdio.h>
#include <iostream>
#include <vector>

//#include "foreach.hpp"
#include "asserts.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_callable_definition.hpp"
#include "formula_constants.hpp"
#include "formula_function.hpp"
#include "formula_tokenizer.hpp"
#include "map_utils.hpp"
#include "random.hpp"
#include "unit_test.hpp"
#include "wml_node.hpp"

namespace {
	//the last formula that was executed; used for outputting debugging info.
	const game_logic::formula* last_executed_formula;
}

void output_formula_error_info() {
	if(last_executed_formula) {
		last_executed_formula->output_debug_info();
	}
}

namespace game_logic
{
	
	formula_error::formula_error()
	{
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
		ASSERT_LOG(false, "Could not get value by slot from formula callable " << slot);
		return variant(0); //so VC++ doesn't complain
	}
	
	map_formula_callable::map_formula_callable(wml::const_node_ptr node)
	: formula_callable(false), fallback_(NULL)
	{
		if(!node) {
			return;
		}
		
		for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
			variant var;
			var.serialize_from_string(i->second);
			add(i->first, var);
		}
	}
	
	void map_formula_callable::write(wml::node_ptr node) const
	{
		for(std::map<std::string,variant>::const_iterator i = values_.begin(); i != values_.end(); ++i) {
			std::string val;
			i->second.serialize_to_string(val);
			node->set_attr(i->first, val);
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
			//a special version of static evaluation that doesn't save a
			//reference to the list, so that we can allow static evaluation
			//not to be fooled.
			variant static_evaluate(const formula_callable& variables) const {
				variant result;
				std::vector<variant>& res = result.initialize_list();
				for(std::vector<expression_ptr>::const_iterator i = items_.begin(); i != items_.end(); ++i) {
					res.push_back((*i)->evaluate(variables));
				}

				return result;
			}

			variant execute(const formula_callable& variables) const {
				std::vector<variant>& res = result_.initialize_list();
				for(std::vector<expression_ptr>::const_iterator i = items_.begin(); i != items_.end(); ++i) {
					res.push_back((*i)->evaluate(variables));
				}
				
				return result_;
			}
			
			std::vector<expression_ptr> items_;
			mutable variant result_;
		};
		
		class map_expression : public formula_expression {
		public:
			explicit map_expression(const std::vector<expression_ptr>& items)
			: formula_expression("_map"), items_(items)
			{}
			
		private:
			variant execute(const formula_callable& variables) const {
				std::map<variant,variant> res;
				for(std::vector<expression_ptr>::const_iterator i = items_.begin(); ( i != items_.end() ) && ( i+1 != items_.end() ) ; i+=2) {
					variant key = (*i)->evaluate(variables);
					variant value = (*(i+1))->evaluate(variables);
					res[ key ] = value;
				}
				
				return variant(&res);
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
					std::cerr << "illegal unary operator: '" << op << "'\n";
					throw formula_error();
				}
			}
		private:
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
					return variant(list_.num_elements());
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
			
			variant v_;
		};
	
		class slot_identifier_expression : public formula_expression {
		public:
			slot_identifier_expression(const std::string& id, int slot, const formula_callable_definition* callable_def)
			: formula_expression("_id"), slot_(slot), id_(id), callable_def_(callable_def)
			{
			}
			
			const std::string& id() const { return id_; }

			bool is_identifier(std::string* ident) const {
				if(ident) {
					*ident = id_;
				}

				return true;
			}

			const formula_callable_definition* get_type_definition() const {
				return callable_def_->get_entry(slot_)->type_definition;
			}
		private:
			variant execute_member(const formula_callable& variables, std::string& id) const {
				id = id_;
				return variables.query_value("self");
			}
			
			variant execute(const formula_callable& variables) const {
//				std::cerr << "ID by slot\n";
				return variables.query_value_by_slot(slot_);
			}

			int slot_;
			std::string id_;
			const formula_callable_definition* callable_def_;
		};
	
		
		class identifier_expression : public formula_expression {
		public:
			identifier_expression(const std::string& id, const formula_callable_definition* callable_def)
			: formula_expression("_id"), id_(id), callable_def_(callable_def)
			{}
			
			const std::string& id() const { return id_; }

			bool is_identifier(std::string* ident) const {
				if(ident) {
					*ident = id_;
				}

				return true;
			}

			expression_ptr optimize() const {
				if(callable_def_) {
					const int index = callable_def_->get_slot(id_);
					if(index != -1) {
						return expression_ptr(new slot_identifier_expression(id_, index, callable_def_));
					}
				}

				return expression_ptr();
			}

			const formula_callable_definition* get_type_definition() const {
				if(callable_def_) {
					const formula_callable_definition::entry* e = callable_def_->get_entry(callable_def_->get_slot(id_));
					if(e) {
						return e->type_definition;
					}
				}

				return NULL;
			}

		private:
			variant execute_member(const formula_callable& variables, std::string& id) const {
				id = id_;
				return variables.query_value("self");
			}
			
			variant execute(const formula_callable& variables) const {
//				std::cerr << "ID by str: '" << id_ << "'\n";
				return variables.query_value(id_);
			}
			std::string id_;
			const formula_callable_definition* callable_def_;
		};
		
		class lambda_function_expression : public formula_expression {
		public:
			lambda_function_expression(const std::vector<std::string>& args, const_formula_ptr fml) : args_(args), fml_(fml)
			{}
			
		private:
			variant execute(const formula_callable& variables) const {
				return variant(fml_, args_, variables);
			}
			
			std::vector<std::string> args_;
			game_logic::const_formula_ptr fml_;
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
				
				return left(args);
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
						list_callable lc(left);	
						return right_->evaluate(lc);
					} else if(left.is_map()) {
						return left[variant(right_->str())];
					}
					
					return left;
				}
				
				return right_->evaluate(*left.as_callable());
			}
			
			variant execute_member(const formula_callable& variables, std::string& id) const {
				const variant left = left_->evaluate(variables);
				
				const identifier_expression* id_expr = dynamic_cast<identifier_expression*>(right_.get());
				if(!id_expr) {
					return right_->evaluate_with_member(*left.as_callable(), id);
				}
				
				id = id_expr->id();
				
				return left;
			}
			
			expression_ptr left_, right_;
		};
		
		class square_bracket_expression : public formula_expression { //TODO
		public:
			square_bracket_expression(expression_ptr left, expression_ptr key)
			: formula_expression("_sqbr"), left_(left), key_(key)
			{}
		private:
			variant execute(const formula_callable& variables) const {
				const variant left = left_->evaluate(variables);
				const variant key = key_->evaluate(variables);
				if(left.is_list() || left.is_map()) {
					return left[ key ];
				} else {
					std::cerr << "illegal usage of operator []'\n";
					throw formula_error();
				}
			}
			
			expression_ptr left_, key_;
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
				}
			}
			
		private:
			variant execute(const formula_callable& variables) const {
				const variant left = left_->evaluate(variables);
				const variant right = right_->evaluate(variables);
				switch(op_) {
					case OP_IN:
						if(!right.is_list()) {
							return variant();
						} else {
							for(int n = 0; n != right.num_elements(); ++n) {
								if(left == right[n]) {
									return variant(1);
								}
							}
							
							return variant(0);
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
						return left / right;
					case OP_POW: 
						return left ^ right;
					case OP_EQ:  
						return left == right ? variant(1) : variant(0);
					case OP_NEQ: 
						return left != right ? variant(1) : variant(0);
					case OP_LTE: 
						return left <= right ? variant(1) : variant(0);
					case OP_GTE: 
						return left >= right ? variant(1) : variant(0);
					case OP_LT:  
						return left < right ? variant(1) : variant(0);
					case OP_GT:  
						return left > right ? variant(1) : variant(0);
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
			
			enum OP { OP_IN, OP_AND, OP_OR, OP_NEQ, OP_LTE, OP_GTE, OP_GT='>', OP_LT='<', OP_EQ='=',
				OP_ADD='+', OP_SUB='-', OP_MUL='*', OP_DIV='/', OP_DICE='d', OP_POW='^', OP_MOD='%' };
			
			OP op_;
			expression_ptr left_, right_;
		};
		
		typedef std::map<std::string,expression_ptr> expr_table;
		typedef boost::shared_ptr<expr_table> expr_table_ptr;
		
		class where_variables: public formula_callable {
		public:
			where_variables(const formula_callable &base,
							expr_table_ptr table )
			: formula_callable(false), base_(base), table_(table) { }
		private:
			const formula_callable& base_;
			expr_table_ptr table_;
			
			mutable std::map<std::string, variant> results_cache_;
			
			void get_inputs(std::vector<formula_input>* inputs) const {
				for(expr_table::const_iterator i = table_->begin(); i != table_->end(); ++i) {
					inputs->push_back(formula_input(i->first, FORMULA_READ_ONLY));
				}
			}

			variant get_value_by_slot(int slot) const {
				return base_.query_value_by_slot(slot);
			}
			
			variant get_value(const std::string& key) const {
				expr_table::iterator i = table_->find(key);
				if(i != table_->end()) {
					std::map<std::string, variant>::const_iterator itor = results_cache_.find(key);
					if(itor != results_cache_.end()) {
						return itor->second;
					}
					
					variant result = i->second->evaluate(base_);
					results_cache_[key] = result;
					return result;
				}
				return base_.query_value(key);
			}
		};
		
		class where_expression: public formula_expression {
		public:
			explicit where_expression(expression_ptr body,
									  expr_table_ptr clauses)
			: formula_expression("_where"), body_(body), clauses_(clauses)
			{}
			
		private:
			expression_ptr body_;
			expr_table_ptr clauses_;
			
			variant execute(const formula_callable& variables) const {
				formula_callable_ptr wrapped_variables(new where_variables(variables, clauses_));
				return body_->evaluate(*wrapped_variables);
			}
		};
		
		class null_expression : public formula_expression {
		public:
			explicit null_expression() : formula_expression("_null") {}
		private:
			variant execute(const formula_callable& /*variables*/) const {
				return variant();
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
			
			variant i_;
		};
		
		class string_expression : public formula_expression {
		public:
			explicit string_expression(std::string str) : formula_expression("_string")
			{
				std::string::iterator i;
				while((i = std::find(str.begin(), str.end(), '{')) != str.end()) {
					std::string::iterator j = std::find(i, str.end(), '}');
					if(j == str.end()) {
						break;
					}
					
					const std::string formula_str(i+1, j);
					const int pos = i - str.begin();
					str.erase(i, j+1);
					
					substitution sub;
					sub.pos = pos;
					sub.calculation.reset(new formula(formula_str));
					subs_.push_back(sub);
				}
				
				std::reverse(subs_.begin(), subs_.end());
				
				str_ = variant(str);
			}

			variant is_literal() const {
				if(subs_.empty()) {
					return variant(str_);
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
				precedence_map["not"] = ++n;
				precedence_map["where"] = ++n;
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
				precedence_map["["]     = ++n;
				precedence_map["("]     = ++n;
				precedence_map["."]     = ++n;
			}
			
			assert(precedence_map.count(std::string(t.begin,t.end)));
			return precedence_map[std::string(t.begin,t.end)];
		}
		
		expression_ptr parse_expression(const token* i1, const token* i2, function_symbol_table* symbols, const formula_callable_definition* callable_def);
		
		void parse_function_args(const token* &i1, const token* i2,
								 std::vector<std::string>* res,
								 std::vector<std::string>* types)
		{
			if(i1->type == TOKEN_LPARENS) {
				++i1;
			} else {
				std::cerr << "Invalid function definition" << std::endl;
				throw formula_error();
			}
			
			while((i1->type != TOKEN_RPARENS) && (i1 != i2)) {
				if(i1->type == TOKEN_IDENTIFIER) {
					if(i1+1 != i2 && std::string((i1+1)->begin, (i1+1)->end) == "*") {
						types->push_back("");
						res->push_back(std::string(i1->begin, i1->end) + std::string("*"));
						++i1;
					} else if(i1+1 != i2 && (i1+1)->type == TOKEN_IDENTIFIER) {
						types->push_back(std::string(i1->begin, i1->end));
						res->push_back(std::string((i1+1)->begin, (i1+1)->end));
						++i1;
					} else {
						types->push_back("");
						res->push_back(std::string(i1->begin, i1->end));
					}
				} else if (i1->type == TOKEN_COMMA) {
					//do nothing
				} else {
					std::cerr << "Invalid function definition" << std::endl;
					throw formula_error();
				}
				++i1;
			}
			
			if(i1->type != TOKEN_RPARENS) {
				std::cerr << "Invalid function definition" << std::endl;
				throw formula_error();
			}
			++i1;
		}
		
		void parse_args(const token* i1, const token* i2,
						std::vector<expression_ptr>* res,
						function_symbol_table* symbols,
						const formula_callable_definition* callable_def)
		{
			int parens = 0;
			const token* beg = i1;
			while(i1 != i2) {
				if(i1->type == TOKEN_LPARENS || i1->type == TOKEN_LSQUARE || i1->type == TOKEN_LBRACKET ) {
					++parens;
				} else if(i1->type == TOKEN_RPARENS || i1->type == TOKEN_RSQUARE || i1->type == TOKEN_RBRACKET) {
					--parens;
				} else if(i1->type == TOKEN_COMMA && !parens) {
					res->push_back(parse_expression(beg,i1, symbols, callable_def));
					beg = i1+1;
				}
				
				++i1;
			}
			
			if(beg != i1) {
				res->push_back(parse_expression(beg,i1, symbols, callable_def));
			}
		}
		
		void parse_set_args(const token* i1, const token* i2,
							std::vector<expression_ptr>* res,
							function_symbol_table* symbols)
		{
			int parens = 0;
			bool check_pointer = false;
			const token* beg = i1;
			while(i1 != i2) {
				if(i1->type == TOKEN_LPARENS || i1->type == TOKEN_LSQUARE) {
					++parens;
				} else if(i1->type == TOKEN_RPARENS || i1->type == TOKEN_RSQUARE) {
					--parens;
				} else if( i1->type == TOKEN_POINTER && !parens ) {
					if (!check_pointer) {
						check_pointer = true;
						res->push_back(parse_expression(beg,i1, symbols, NULL));
						beg = i1+1;
					} else {
						std::cerr << "Too many '->' operators\n";
						throw formula_error();
					}
				} else if( i1->type == TOKEN_COMMA && !parens ) {
					check_pointer = false;
					res->push_back(parse_expression(beg,i1, symbols, NULL));
					beg = i1+1;
				}
				
				++i1;
			}
			
			if(beg != i1) {
				res->push_back(parse_expression(beg,i1, symbols, NULL));
			}
		}
		
		void parse_where_clauses(const token* i1, const token * i2,
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
							std::cerr << "There is 'where <expression>,; "
							<< "'where name=<expression>,' was needed.\n";
							throw formula_error();
						}
						(*res)[var_name] = parse_expression(beg,i1, symbols, callable_def);
						beg = i1+1;
						var_name = "";
					} else if(i1->type == TOKEN_OPERATOR) {
						std::string op_name(i1->begin, i1->end);
						if(op_name == "=") {
							if(beg->type != TOKEN_IDENTIFIER) {
								if(i1 == original_i1_cached) {
									std::cerr<< "There is 'where =<expression'; "
									<< "'where name=<expression>' was needed.\n";
								} else {
									std::cerr<< "There is 'where <expression>=<expression>'; "
									<< "'where name=<expression>' was needed.\n";
								}
								throw formula_error();
							} else if(beg+1 != i1) {
								std::cerr<<"There is 'where name <expression>=<expression>'; "
								<< "'where name=<expression>' was needed.\n";
								throw formula_error();
							} else if(!var_name.empty()) {
								std::cerr<<"There is 'where name=name=<expression>'; "
								<<"'where name=<expression>' was needed.\n";
								throw formula_error();
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
					std::cerr << "There is 'where <expression>'; "
					<< "'where name=<expression> was needed.\n";
					throw formula_error();
				}
				(*res)[var_name] = parse_expression(beg,i1, symbols, callable_def);
			}
		}
		
		expression_ptr parse_expression_internal(const token* i1, const token* i2, function_symbol_table* symbols, const formula_callable_definition* callable_def);
		
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
				static_formula_callable() {
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

		expression_ptr optimize_expression(expression_ptr result, function_symbol_table* symbols, const formula_callable_definition* callable_def)
		{
			const std::string str = result->str();

			//we want to try to evaluate this expression, and see if it is static.
			//it is static if it never reads its input, if it doesn't call the rng,
			//and if a reference to the input itself is not stored.
			try {
				const unsigned int rng_seed = rng::get_seed();
				formula_callable_ptr static_callable(new static_formula_callable);
				variant res = result->static_evaluate(*static_callable);
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

			if(result) {
				expression_ptr optimized = result->optimize();
				if(optimized) {
					result = optimized;
				}
			}
			
			if(result) {
				result->set_str(str);
			}

			return result;
		}
		
		expression_ptr parse_expression(const token* i1, const token* i2, function_symbol_table* symbols, const formula_callable_definition* callable_def)
		{
			expression_ptr result(parse_expression_internal(i1, i2, symbols, callable_def));
			result->set_str(std::string(i1->begin, (i2-1)->end));
			result = optimize_expression(result, symbols, callable_def);

			return result;
		}
		
		expression_ptr parse_expression_internal(const token* i1, const token* i2, function_symbol_table* symbols, const formula_callable_definition* callable_def)
		{
			if(i1 == i2) {
				std::cerr << "empty expression\n";
				throw formula_error();
			}
			
			if(i1->type == TOKEN_KEYWORD && std::string(i1->begin, i1->end) == "def" &&
			   ((i1+1)->type == TOKEN_IDENTIFIER || (i1+1)->type == TOKEN_LPARENS)) {
				++i1;
				std::string formula_name;
				if(i1->type == TOKEN_IDENTIFIER) {
					formula_name = std::string(i1->begin, i1->end);
					++i1;
				}
				
				std::vector<std::string> args, types;
				parse_function_args(i1, i2, &args, &types);
				const token* beg = i1;
				while((i1 != i2) && (i1->type != TOKEN_SEMICOLON)) {
					++i1;
				}
				const std::string formula_str = std::string(beg->begin, (i1-1)->end);
				
				recursive_function_symbol_table recursive_symbols(formula_name.empty() ? "recurse" : formula_name, args, symbols);

				formula_callable_definition_ptr args_definition;
				if(formula_name.empty() == false) {
					//create a definition of the callable representing
					//function arguments.
					args_definition = create_formula_callable_definition(&args[0], &args[0] + args.size());
					for(int n = 0; n != types.size(); ++n) {
						if(types[n].empty()) {
							continue;
						}

						ASSERT_LOG(args_definition->get_entry(n) != NULL, "FORMULA FUNCTION TYPE ARGS MIS-MATCH");

						const formula_callable_definition* def = get_formula_callable_definition(types[n]);
						ASSERT_LOG(def != NULL, "TYPE NOT FOUND: " << types[n]);
						args_definition->get_entry(n)->type_definition = def;
					}
				}

				const_formula_ptr fml(new formula(formula_str, &recursive_symbols, args_definition.get()));
				recursive_symbols.resolve_recursive_calls(fml);
				
				if(formula_name.empty()) {
					return expression_ptr(new lambda_function_expression(args, fml));
				}
				
				const std::string precond = "";
				symbols->add_formula_function(formula_name, fml,
											  formula::create_optional_formula(precond, symbols), args);
				if((i1 == i2) || (i1 == (i2-1))) {
					return expression_ptr(new function_list_expression(symbols));
				}
				else {
					return parse_expression((i1+1), i2, symbols, callable_def);
				}
			}
			
			int parens = 0;
			const token* op = NULL;
			const token* fn_call = NULL;
			
			for(const token* i = i1; i != i2; ++i) {
				if(fn_call && i+1 == i2 && i->type != TOKEN_RPARENS) {
					fn_call = NULL;
				}
				
				if(i->type == TOKEN_LPARENS || i->type == TOKEN_LSQUARE) {
					if(i->type == TOKEN_LPARENS && parens == 0 && i != i1) {
						fn_call = i;
					} else if(i->type == TOKEN_LSQUARE && parens == 0 && i != i1 && (i-1)->type != TOKEN_OPERATOR && (op == NULL || operator_precedence(*op) > operator_precedence(*i))) {
						//the square bracket itself is an operator
						op = i;
					}
					
					++parens;
				} else if(i->type == TOKEN_RPARENS || i->type == TOKEN_RSQUARE) {
					--parens;
					
					if(parens == 0 && i+1 != i2) {
						fn_call = NULL;
					}
				} else if(parens == 0 && i->type == TOKEN_OPERATOR) {
					if(op == NULL || operator_precedence(*op) >
					   operator_precedence(*i)) {
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
					return parse_expression(i1+1,i2-1,symbols, callable_def);
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
							//create a list
							std::vector<expression_ptr> args;
							parse_args(i1+1,i2-1,&args,symbols, callable_def);
							return expression_ptr(new list_expression(args));
						} else {
							//execute operator [ ]
							return expression_ptr(new square_bracket_expression(
																				parse_expression(i1,tok,symbols, callable_def),
																				parse_expression(tok+1,i2-1,symbols, callable_def)));
						}
					}
				} else if(i1->type == TOKEN_LBRACKET && (i2-1)->type == TOKEN_RBRACKET) {
					//create a map TODO: add support for a set
					std::vector<expression_ptr> args;
					parse_set_args(i1+1,i2-1,&args,symbols);
					return expression_ptr(new map_expression(args));
				} else if(i2 - i1 == 1) {
					if(i1->type == TOKEN_KEYWORD) {
						if(std::string(i1->begin,i1->end) == "functions") {
							return expression_ptr(new function_list_expression(symbols));
						}
					} else if(i1->type == TOKEN_CONST_IDENTIFIER) {
						return expression_ptr(new const_identifier_expression(
																			  std::string(i1->begin,i1->end)));
					} else if(i1->type == TOKEN_IDENTIFIER) {
						return expression_ptr(new identifier_expression(
																		std::string(i1->begin,i1->end), callable_def));
					} else if(i1->type == TOKEN_INTEGER) {
						int n = strtol(std::string(i1->begin,i1->end).c_str(), NULL, 0);
						return expression_ptr(new integer_expression(n));
					} else if(i1->type == TOKEN_STRING_LITERAL) {
						return expression_ptr(new string_expression(std::string(i1->begin+1,i1->end-1)));
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
						const bool optimize = optimize_function_arguments(function_name, symbols);
						parse_args(i1+2,i2-1,&args,symbols, optimize ? callable_def : NULL);
						expression_ptr result(create_function(function_name, args, symbols, callable_def));
						if(result) {
							return result;
						}
					}
				}
				
				if(!fn_call) {
					std::ostringstream expr;
					while(i1 != i2) {
						expr << std::string(i1->begin,i1->end);
						++i1;
					}
					std::cerr << "could not parse expression: '" << expr.str() << "'\n";
					throw formula_error();
				}
			}
			
			if(fn_call && (op == NULL || operator_precedence(*op) >
						   operator_precedence(*fn_call))) {
				op = fn_call;
			}
			
			if(op == i1) {
				return expression_ptr(new unary_operator_expression(
																	std::string(op->begin,op->end),
																	parse_expression(op+1,i2,symbols, callable_def)));
			}
			
			const std::string op_name(op->begin,op->end);
			
			if(op_name == "(") {
				std::vector<expression_ptr> args;
				parse_args(op+1, i2-1, &args, symbols, callable_def);
				
				return expression_ptr(new function_call_expression(
																   parse_expression(i1, op, symbols, callable_def), args));
			}
			
			if(op_name == ".") {
				expression_ptr left(parse_expression(i1,op,symbols, callable_def));
				const formula_callable_definition* type_definition = left->get_type_definition();
				//TODO: right now we don't give the right side of the dot
				//a expression definition. We should work out if we can
				//statically derive information from the left half to
				//give the right half a definition.
				return expression_ptr(new dot_expression(left,
														 parse_expression(op+1,i2,symbols, type_definition)));
			}
			
			if(op_name == "where") {
				expr_table_ptr table(new expr_table());
				parse_where_clauses(op+1, i2, table, symbols, callable_def);
				return expression_ptr(new where_expression(parse_expression(i1, op, symbols, callable_def),
														   table));
			}

			const bool is_dot = op_name == ".";
			return expression_ptr(new operator_expression(
														  op_name, parse_expression(i1,op,symbols, callable_def),
														  parse_expression(op+1,i2,symbols, callable_def)));
		}
		
	}
	
	formula_ptr formula::create_string_formula(const std::string& str)
	{
		formula_ptr res(new formula());
		res->expr_.reset(new string_expression(str));
		return res;
	}
	
	formula_ptr formula::create_optional_formula(const wml::value& val, function_symbol_table* symbols, const formula_callable_definition* callable_definition)
	{
		if(val.empty()) {
			return formula_ptr();
		}
		
		try {
			return formula_ptr(new formula(val, symbols, callable_definition));
		} catch(...) {
			if(val.filename()) {
				std::cerr << *val.filename() << " " << val.line() << ": ";
			}
			
			std::cerr << "ERROR parsing optional formula: '" << val << "'\n";
			//for now die a horrible death on such errors
			assert(false);
			
			return formula_ptr();
		}
	}
	
	formula::formula(const wml::value& val, function_symbol_table* symbols, const formula_callable_definition* callable_definition) : str_(val.str()), filename_(val.filename()), line_(val.line())
	{
		using namespace formula_tokenizer;
		
		std::vector<token> tokens;
		std::string::const_iterator i1 = str_.begin(), i2 = str_.end();
		while(i1 != i2) {
			try {
				tokens.push_back(get_token(i1,i2));
				if((tokens.back().type == TOKEN_WHITESPACE) || (tokens.back().type == TOKEN_COMMENT)) {
					tokens.pop_back();
				}
			} catch(token_error& /*e*/) {
				throw formula_error();
			}
		}
		
		try {
			if(tokens.size() != 0) {
				expr_ = parse_expression(&tokens[0],&tokens[0] + tokens.size(), symbols, callable_definition);
			} else {
				expr_ = expression_ptr(new null_expression());
			}	
		} catch(formula_error&) {
			std::cerr << "ERROR WHILE PARSING AT " << (filename_ ? *filename_ : "UNKNOWN") << ":" << line_ << "::\n" << str_ << "\n";
			throw;
		}
	}
	
	formula::~formula() {
		if(last_executed_formula == this) {
			last_executed_formula = NULL;
		}
	}
	
	void formula::output_debug_info() const
	{
		std::cerr << "FORMULA: ";
		if(filename_) {
			std::cerr << *filename_ << " " << line_ << ": ";
		}
		
		std::cerr << str_ << "\n";
	}
	
	variant formula::execute(const formula_callable& variables) const
	{
		last_executed_formula = this;
		try {
			return expr_->evaluate(variables);
		} catch(type_error& e) {
			if(filename_) {
				std::cerr << *filename_ << " " << line_ << ": ";
			}
			std::cerr << "formula type error: " << e.message << "\n";
			
			//for now die a horrible death on a formula type error
			assert(false);
			return variant();
		}
	}
	
	variant formula::execute() const
	{
		last_executed_formula = this;
		
		map_formula_callable* null_callable = new map_formula_callable;
		variant ref(null_callable);
		return execute(*null_callable);
	}
	
	UNIT_TEST(formula_in) {
		CHECK(formula("1 in [4,5,6]").execute() == variant(0), "test failed");
		CHECK(formula("5 in [4,5,6]").execute() == variant(1), "test failed");
	}
	
	UNIT_TEST(formula_fn) {
		function_symbol_table symbols;
		CHECK(formula("def f(g) g(5) + 1; f(def(n) n*n)", &symbols).execute() == variant(26), "test failed");
	}
	
	UNIT_TEST(array_index) {
		formula f("map(range(6), 'n', elements[n]) = elements "
				  "where elements = [5, 6, 7, 8, 9, 10]");
		CHECK(f.execute() == variant(1), "test failed");
	}
	
	UNIT_TEST(dot_precedence) {
		map_formula_callable* callable = new map_formula_callable;
		variant ref(callable);
		map_formula_callable* callable2 = new map_formula_callable;
		std::vector<variant> v;
		for(int n = 0; n != 10; ++n) {
			v.push_back(variant(n));
		}
		callable2->add("item", variant(&v));
		callable->add("obj", variant(callable2));
		formula f("obj.item[n] where n = 2");
		const variant result = f.execute(*callable);
		CHECK(result == variant(2), "test failed: " << result.to_debug_string());
	}
	
}

#ifdef UNIT_TEST_FORMULA
using namespace game_logic;
class mock_char : public formula_callable {
	variant get_value(const std::string& key) const {
		if(key == "strength") {
			return variant(15);
		} else if(key == "agility") {
			return variant(12);
		}
		
		return variant(10);
	}
};
class mock_party : public formula_callable {
	variant get_value(const std::string& key) const {
		c_.add_ref();
		i_[0].add_ref();
		i_[1].add_ref();
		i_[2].add_ref();
		if(key == "members") {
			i_[0].add("strength",variant(12));
			i_[1].add("strength",variant(16));
			i_[2].add("strength",variant(14));
			std::vector<variant> members;
			for(int n = 0; n != 3; ++n) {
				members.push_back(variant(&i_[n]));
			}
			
			return variant(&members);
		} else if(key == "char") {
			return variant(&c_);
		} else {
			return variant(0);
		}
	}
	
	mock_char c_;
	mutable map_formula_callable i_[3];
	
};

#include <time.h>
#include <sys/times.h>

void benchmark_formula(const std::string& formula_str, formula_callable& callable)
{
	std::cerr << "benchmarking " << formula_str << "...\n";
	
	formula f(formula_str);
	const int ticks_second = sysconf(_SC_CLK_TCK);
	
	tms start, end;
	times(&start);
	
	for(int n = 0; n != 1000000; ++n) {
		f.execute(callable);
	}
	
	times(&end);
	const int ticks = ((end.tms_utime + end.tms_stime) -
					   (start.tms_utime + start.tms_stime));
	const int ns = (ticks*1000)/ticks_second;
	std::cerr << formula_str << ": " << ns << "ns\n";
}

int main()
{
	srand(time(NULL));
	mock_char c;
	mock_party p;
	try {
		assert(formula("strength").execute(c).as_int() == 15);
		assert(formula("17").execute(c).as_int() == 17);
		assert(formula("strength/2 + agility").execute(c).as_int() == 19);
		assert(formula("(strength+agility)/2").execute(c).as_int() == 13);
		assert(formula("strength > 12").execute(c).as_int() == 1);
		assert(formula("strength > 18").execute(c).as_int() == 0);
		assert(formula("if(strength > 12, 7, 2)").execute(c).as_int() == 7);
		assert(formula("if(strength > 18, 7, 2)").execute(c).as_int() == 2);
		assert(formula("2 and 1").execute(c).as_int() == 1);
		assert(formula("2 and 0").execute(c).as_int() == 0);
		assert(formula("2 or 0").execute(c).as_int() == 2);
		assert(formula("-5").execute(c).as_int() == -5);
		assert(formula("not 5").execute(c).as_int() == 0);
		assert(formula("not 0").execute(c).as_int() == 1);
		assert(formula("abs(5)").execute(c).as_int() == 5);
		assert(formula("abs(-5)").execute(c).as_int() == 5);
		assert(formula("min(3,5)").execute(c).as_int() == 3);
		assert(formula("min(5,2)").execute(c).as_int() == 2);
		assert(formula("max(3,5)").execute(c).as_int() == 5);
		assert(formula("max(5,2)").execute(c).as_int() == 5);
		assert(formula("max(4,5,[2,18,7])").execute(c).as_int() == 18);
		assert(formula("char.strength").execute(p).as_int() == 15);
		assert(formula("choose(members,strength).strength").execute(p).as_int() == 16);
		assert(formula("4^2").execute().as_int() == 16);
		assert(formula("2+3^3").execute().as_int() == 29);
		assert(formula("2*3^3+2").execute().as_int() == 56);
		assert(formula("9^3").execute().as_int() == 729);
		assert(formula("x*5 where x=1").execute().as_int() == 5);
		assert(formula("x*(a*b where a=2,b=1) where x=5").execute().as_int() == 10);
		assert(formula("char.strength * ability where ability=3").execute(p).as_int() == 45);
		assert(formula("'abcd' = 'abcd'").execute(p).as_bool() == true);
		assert(formula("'abcd' = 'acd'").execute(p).as_bool() == false);
		assert(formula("'strength, agility: {strength}, {agility}'").execute(c).as_string() ==
			   "strength, agility: 15, 12");
		const int dice_roll = formula("3d6").execute().as_int();
		assert(dice_roll >= 3 && dice_roll <= 18);
		
		assert(formula::create_string_formula("Your strength is {strength}")->execute(c).as_string() ==
			   "Your strength is 15");
		variant myarray = formula("[1,2,3]").execute();
		assert(myarray.num_elements() == 3);
		assert(myarray[0].as_int() == 1);
		assert(myarray[1].as_int() == 2);
		assert(myarray[2].as_int() == 3);
		
		benchmark_formula("1", c);
		benchmark_formula("5 + 4", c);
		benchmark_formula("(strength+agility)/2", c);
		benchmark_formula("sum([4, 5, 8, 12, 10])", c);
		benchmark_formula("sum(map([4, 5, 8, 12, 10], 'n', n*n))", c);
	} catch(formula_error& e) {
		std::cerr << "parse error\n";
	}
}
#endif
