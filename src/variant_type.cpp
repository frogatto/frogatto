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
#include <vector>

#include <stdio.h>

#include "asserts.hpp"
#include "foreach.hpp"
#include "formula_function.hpp"
#include "formula_object.hpp"
#include "formula_tokenizer.hpp"
#include "unit_test.hpp"
#include "variant_type.hpp"

variant_type::variant_type()
{
}

variant_type::~variant_type()
{
}

namespace {

class variant_type_simple : public variant_type
{
public:
	variant_type_simple(const variant& original_str, const formula_tokenizer::token& tok)
	  : type_(variant::string_to_type(std::string(tok.begin, tok.end)))
	{
		ASSERT_LOG(type_ != variant::VARIANT_TYPE_INVALID, "INVALID TYPE: " << std::string(tok.begin, tok.end) << " AT:\n" << game_logic::pinpoint_location(original_str, tok.begin, tok.end));
	}

	explicit variant_type_simple(variant::TYPE type) : type_(type) {}

	bool match(const variant& v) const {
		return v.type() == type_ || type_ == variant::VARIANT_TYPE_DECIMAL && v.type() == variant::VARIANT_TYPE_INT;
	}

	bool is_type(variant::TYPE type) const {
		return type == type_;
	}

	bool is_equal(const variant_type& o) const {
		const variant_type_simple* other = dynamic_cast<const variant_type_simple*>(&o);
		if(!other) {
			return false;
		}

		return type_ == other->type_;
	}

	std::string to_string() const {
		return variant::variant_type_to_string(type_);
	}

	variant_type_ptr is_list_of() const {
		if(type_ == variant::VARIANT_TYPE_LIST) {
			return variant_type::get_any();
		} else {
			return variant_type_ptr();
		}
	}

	std::pair<variant_type_ptr, variant_type_ptr> is_map_of() const {
		if(type_ == variant::VARIANT_TYPE_MAP) {
			return std::pair<variant_type_ptr, variant_type_ptr>(variant_type::get_any(), variant_type::get_any());
		} else {
			return std::pair<variant_type_ptr, variant_type_ptr>();
		}
	}

	bool is_compatible(variant_type_ptr type) const {
		const variant_type_simple* simple_type = dynamic_cast<const variant_type_simple*>(type.get());
		if(simple_type && simple_type->type_ == type_) {
			return true;
		}

		if(type_ == variant::VARIANT_TYPE_DECIMAL) {
			if(simple_type && simple_type->type_ == variant::VARIANT_TYPE_INT) { 
				return true;
			}
		} else if(type_ == variant::VARIANT_TYPE_LIST) {
			if(type->is_list_of()) {
				return true;
			}
		} else if(type_ == variant::VARIANT_TYPE_MAP) {
			if(type->is_map_of().first) {
				return true;
			}
		}

		return false;
	}

private:
	int order_id() const { return 1; }
	variant::TYPE type_;
};

class variant_type_any : public variant_type
{
public:
	bool match(const variant& v) const { return true; }
	bool is_equal(const variant_type& o) const {
		const variant_type_any* other = dynamic_cast<const variant_type_any*>(&o);
		return other != NULL;
	}

	std::string to_string() const {
		return "any";
	}

	bool is_compatible(variant_type_ptr type) const {
		return true;
	}

	bool is_any() const { return true; }
private:
	int order_id() const { return 2; }
};

class variant_type_commands : public variant_type
{
public:
	bool match(const variant& v) const {
		if(v.is_null()) {
			return true;
		}

		if(v.try_convert<game_logic::command_callable>()) {
			return true;
		}

		if(v.is_list()) {
			for(int n = 0; n != v.num_elements(); ++n) {
				if(!match(v[n])) {
					return false;
				}
			}
		}

		return false;
	}
	bool is_equal(const variant_type& o) const {
		const variant_type_commands* other = dynamic_cast<const variant_type_commands*>(&o);
		return other != NULL;
	}

	std::string to_string() const {
		return "commands";
	}

	bool is_compatible(variant_type_ptr type) const {
		return is_equal(*type);
	}
private:
	int order_id() const { return 12; }
};

class variant_type_class : public variant_type
{
public:
	explicit variant_type_class(const std::string& type) : type_(type)
	{
		ASSERT_LOG(game_logic::formula_class_valid(type), "INVALID FORMULA CLASS: " << type);
	}

	bool match(const variant& v) const {
		const game_logic::formula_object* obj = v.try_convert<game_logic::formula_object>();
		if(!obj) {
			return false;
		}

		return obj->is_a(type_);
	}

	bool is_class(std::string* class_name) const {
		if(class_name) {
			*class_name = type_;
		}

		return true;
	}

	bool is_equal(const variant_type& o) const {
		const variant_type_class* other = dynamic_cast<const variant_type_class*>(&o);
		if(!other) {
			return false;
		}

		return type_ == other->type_;
	}

	std::string to_string() const {
		return "class " + type_;
	}

	bool is_compatible(variant_type_ptr type) const {
		const variant_type_class* class_type = dynamic_cast<const variant_type_class*>(type.get());
		if(class_type) {
			return game_logic::is_class_derived_from(class_type->type_, type_);
		} else if(type->is_type(variant::VARIANT_TYPE_MAP)) {
			//maps can be converted implicity to class type.
			return true;
		}

		return false;
	}
private:
	std::string type_;
	int order_id() const { return 3; }
};

class variant_type_union : public variant_type
{
public:
	explicit variant_type_union(const std::vector<variant_type_ptr>& v) : types_(v)
	{}
	bool match(const variant& v) const {
		foreach(const variant_type_ptr& p, types_) {
			if(p->match(v)) {
				return true;
			}
		}

		return false;
	}

	bool is_equal(const variant_type& o) const {
		const variant_type_union* other = dynamic_cast<const variant_type_union*>(&o);
		if(!other) {
			return false;
		}

		if(types_.size() != other->types_.size()) {
			return false;
		}

		for(int n = 0; n != types_.size(); ++n) {
			if(types_[n]->is_equal(*other->types_[n]) == false) {
				return false;
			}
		}

		return true;
	}

	std::string to_string() const {
		std::string result;
		for(int n = 0; n != types_.size(); ++n) {
			if(n != 0) {
				result += "|";
			}

			result += types_[n]->to_string();
		}
		return result;
	}

	bool is_function(std::vector<variant_type_ptr>* args, variant_type_ptr* return_type, int* min_args) const
	{
		std::vector<std::vector<variant_type_ptr> > arg_lists(types_.size());
		std::vector<variant_type_ptr> return_types(types_.size());
		std::vector<int> min_args_list(types_.size());

		int max_min_args = -1;
		int num_args = 0;
		for(int n = 0; n != types_.size(); ++n) {
			if(!types_[n]->is_function(&arg_lists[n], &return_types[n], &min_args_list[n])) {
				return false;
			}

			if(max_min_args == -1 || min_args_list[n] > max_min_args) {
				max_min_args = min_args_list[n];
			}

			if(arg_lists[n].size() > num_args) {
				num_args = arg_lists[n].size();
			}
		}

		if(args) {
			args->clear();
			for(int n = 0; n != num_args; ++n) {
				std::vector<variant_type_ptr> a;
				foreach(const std::vector<variant_type_ptr>& arg, arg_lists) {
					if(n < arg.size()) {
						a.push_back(arg[n]);
					}
				}

				args->push_back(get_union(a));
			}
		}

		if(return_type) {
			*return_type = get_union(return_types);
		}

		if(min_args) {
			*min_args = max_min_args;
		}

		return true;
	}

	const std::vector<variant_type_ptr>* is_union() const { return &types_; }
private:
	variant_type_ptr null_excluded() const {
		std::vector<variant_type_ptr> new_types;
		foreach(variant_type_ptr t, types_) {
			if(t->is_type(variant::VARIANT_TYPE_NULL) == false) {
				new_types.push_back(t);
			}
		}

		if(new_types.size() != types_.size()) {
			return get_union(new_types);
		} else {
			return variant_type_ptr();
		}
	}

	std::vector<variant_type_ptr> types_;
	int order_id() const { return 4; }
};

class variant_type_list : public variant_type
{
public:
	explicit variant_type_list(const variant_type_ptr& value) : value_type_(value)
	{}

	bool match(const variant& v) const {
		if(!v.is_list()) {
			return false;
		}

		for(int n = 0; n != v.num_elements(); ++n) {
			if(!value_type_->match(v[n])) {
				return false;
			}
		}

		return true;
	}

	bool is_equal(const variant_type& o) const {
		const variant_type_list* other = dynamic_cast<const variant_type_list*>(&o);
		if(!other) {
			return false;
		}

		return value_type_->is_equal(*other->value_type_);
	}

	std::string to_string() const {
		return "[" + value_type_->to_string() + "]";
	}

	variant_type_ptr is_list_of() const {
		return value_type_;
	}

	bool is_compatible(variant_type_ptr type) const {
		variant_type_ptr value_type = type->is_list_of();
		if(value_type) {
			return variant_types_compatible(value_type_, value_type);
		}

		if(type->is_type(variant::VARIANT_TYPE_LIST)) {
			return variant_types_compatible(value_type_, variant_type::get_any());
		}

		return false;

	}
private:
	variant_type_ptr value_type_;
	int order_id() const { return 5; }
};

class variant_type_map : public variant_type
{
public:
	variant_type_map(variant_type_ptr key, variant_type_ptr value)
	  : key_type_(key), value_type_(value)
	{}

	bool match(const variant& v) const {
		if(!v.is_map()) {
			return false;
		}

		foreach(const variant::map_pair& p, v.as_map()) {
			if(!key_type_->match(p.first) || !value_type_->match(p.second)) {
				return false;
			}
		}

		return true;
	}

	bool is_equal(const variant_type& o) const {
		const variant_type_map* other = dynamic_cast<const variant_type_map*>(&o);
		if(!other) {
			return false;
		}

		return value_type_->is_equal(*other->value_type_) &&
		       key_type_->is_equal(*other->key_type_);
	}
	std::string to_string() const {
		return "{" + key_type_->to_string() + " -> " + value_type_->to_string() + "}";
	}

	std::pair<variant_type_ptr, variant_type_ptr> is_map_of() const {
		return std::pair<variant_type_ptr, variant_type_ptr>(key_type_, value_type_);
	}

	bool is_compatible(variant_type_ptr type) const {
		std::pair<variant_type_ptr,variant_type_ptr> p = type->is_map_of();
		if(p.first && p.second) {
			return variant_types_compatible(key_type_, p.first) &&
			       variant_types_compatible(value_type_, p.second);
		}

		if(type->is_type(variant::VARIANT_TYPE_LIST)) {
			return variant_types_compatible(key_type_, variant_type::get_any()) &&
			       variant_types_compatible(value_type_, variant_type::get_any());
		}

		return false;

	}
private:
	variant_type_ptr key_type_, value_type_;
	int order_id() const { return 6; }
};

class variant_type_function : public variant_type
{
public:
	variant_type_function(const std::vector<variant_type_ptr>& args,
	                      variant_type_ptr return_type, int min_args)
	  : args_(args), return_(return_type), min_args_(min_args)
	{
	}

	~variant_type_function() {
	}

	bool is_function(std::vector<variant_type_ptr>* args, variant_type_ptr* return_type, int* min_args) const
	{
		if(args) {
			*args = args_;
		}

		if(return_type) {
			*return_type = return_;
		}

		if(min_args) {
			*min_args = min_args_;
		}

		return true;
	}

	bool is_equal(const variant_type& o) const {
		const variant_type_function* other = dynamic_cast<const variant_type_function*>(&o);
		if(!other) {
			return false;
		}

		if(!return_->is_equal(*other->return_) || args_.size() != other->args_.size()) {
			return false;
		}

		for(int n = 0; n != args_.size(); ++n) {
			if(!args_[n]->is_equal(*other->args_[n])) {
				return false;
			}
		}

		return true;
	}

	std::string to_string() const {
		std::string result = "function(";
		for(int n = 0; n != args_.size(); ++n) {
			if(n != 0) {
				result += ",";
			}

			result += args_[n]->to_string();
		}

		result += ") -> " + return_->to_string();
		return result;
	}

	bool match(const variant& v) const {
		if(v.is_function() == false) {
			return false;
		}

		if(v.function_return_type()->is_equal(*return_) == false) {
			return false;
		}

		if(v.max_function_arguments() != args_.size() || v.min_function_arguments() != min_args_) {
			return false;
		}

		const std::vector<variant_type_ptr>& arg_types = v.function_arg_types();
		for(int n = 0; n != arg_types.size(); ++n) {
			if(arg_types[n]->is_equal(*args_[n]) == false) {
				return false;
			}
		}

		return true;
	}

	bool is_compatible(variant_type_ptr type) const {
		std::vector<variant_type_ptr> args;
		variant_type_ptr return_type;
		int min_args = 0;
		if(type->is_function(&args, &return_type, &min_args)) {
			if(min_args != min_args_) {
				return false;
			}

			if(!variant_types_compatible(return_, return_type)) {
				return false;
			}

			if(args.size() != args_.size()) {
				return false;
			}

			for(int n = 0; n != args_.size(); ++n) {
				if(!variant_types_compatible(args_[n], args[n])) {
					return false;
				}
			}

			return true;
		}

		return false;
	}
	
private:
	int order_id() const { return 7; }

	std::vector<variant_type_ptr> args_;
	variant_type_ptr return_;
	int min_args_;
};

}

std::string variant_type_is_class_or_null(variant_type_ptr type)
{
	std::string class_name;
	if(type->is_class(&class_name)) {
		return class_name;
	}

	const std::vector<variant_type_ptr>* union_vec = type->is_union();
	if(union_vec) {
		foreach(variant_type_ptr t, *union_vec) {
			bool found_class = false;
			if(class_name.empty()) {
				class_name = variant_type_is_class_or_null(t);
				if(class_name.empty() == false) {
					found_class = true;
				}
			}

			if(found_class == false && t->is_type(variant::VARIANT_TYPE_NULL) == false) {
				return "";
			}
		}
	}

	return class_name;
}

bool variant_types_compatible(variant_type_ptr to, variant_type_ptr from)
{
	if(from->is_union()) {
		foreach(variant_type_ptr from_type, *from->is_union()) {
			if(variant_types_compatible(to, from_type) == false) {
				return false;
			}
		}

		return true;
	}

	if(to->is_union()) {
		foreach(variant_type_ptr to_type, *to->is_union()) {
			if(variant_types_compatible(to_type, from)) {
				return true;
			}
		}

		return false;
	}

	return to->is_compatible(from);
}

variant_type_ptr parse_variant_type(const variant& original_str,
                                    const formula_tokenizer::token*& i1,
                                    const formula_tokenizer::token* i2,
									bool allow_failure)
{
#define ASSERT_COND(cond, msg) if(cond) {} else if(allow_failure) { return variant_type_ptr(); } else { ASSERT_LOG(cond, msg); }

	using namespace formula_tokenizer;

	std::vector<variant_type_ptr> v;

	const token* begin_token = i1;

	for(;;) {
		ASSERT_COND(i1 != i2, "EXPECTED TYPE BUT FOUND EMPTY EXPRESSION:" << original_str.debug_location());
		if(i1->type == TOKEN_IDENTIFIER && i1->equals("class")) {
			++i1;
			ASSERT_COND(i1 != i2, "EXPECTED CLASS BUT FOUND EMPTY EXPRESSION:\n" << game_logic::pinpoint_location(original_str, (i1-1)->end));
			std::string class_name(i1->begin, i1->end);

			while(i1+1 != i2 && i1+2 != i2 && (i1+1)->equals(".")) {
				class_name += ".";
				i1 += 2;
				class_name += std::string(i1->begin, i1->end);
			}
			v.push_back(variant_type_ptr(new variant_type_class(class_name)));

			++i1;
		} else if(i1->type == TOKEN_IDENTIFIER && i1->equals("any")) {
			v.push_back(variant_type_ptr(new variant_type_any));
			++i1;
		} else if(i1->type == TOKEN_IDENTIFIER && i1->equals("commands")) {
			v.push_back(variant_type_ptr(new variant_type_commands));
			++i1;
		} else if(i1->type == TOKEN_IDENTIFIER || (i1->type == TOKEN_KEYWORD && std::equal(i1->begin, i1->end, "null"))) {
			ASSERT_COND(variant::string_to_type(std::string(i1->begin, i1->end)) != variant::VARIANT_TYPE_INVALID,
			  "INVALID TOKEN WHEN PARSING TYPE: " << std::string(i1->begin, i1->end) << " AT:\n" << game_logic::pinpoint_location(original_str, i1->begin, i1->end));
			v.push_back(variant_type_ptr(new variant_type_simple(original_str, *i1)));
			++i1;
		} else if(i1->type == TOKEN_LBRACKET) {
			const token* end = i1+1;
			const bool res = token_matcher().add(TOKEN_RBRACKET).find_match(end, i2);
			ASSERT_COND(res, "ERROR PARSING MAP TYPE: " << original_str.debug_location());

			++i1;
			ASSERT_COND(i1 != end, "ERROR PARSING MAP TYPE: " << original_str.debug_location());

			const variant_type_ptr key_type = parse_variant_type(original_str, i1, end, allow_failure);
			ASSERT_COND(i1->type == TOKEN_POINTER, "ERROR PARSING MAP TYPE, NO ARROW FOUND: " << original_str.debug_location());
		
			++i1;
			ASSERT_COND(i1 != end, "ERROR PARSING MAP TYPE: " << original_str.debug_location());

			const variant_type_ptr value_type = parse_variant_type(original_str, i1, end, allow_failure);
			ASSERT_COND(i1 == end, "ERROR PARSING MAP TYPE: " << original_str.debug_location());

			v.push_back(variant_type_ptr(new variant_type_map(key_type, value_type)));

			++i1;

		} else if(i1->type == TOKEN_LSQUARE) {
			const token* end = i1+1;
			const bool res = token_matcher().add(TOKEN_RSQUARE).find_match(end, i2);
			ASSERT_COND(res, "ERROR PARSING ARRAY TYPE: " << original_str.debug_location());
	
			++i1;
			ASSERT_COND(i1 != end, "ERROR PARSING ARRAY TYPE: " << original_str.debug_location());
			
			const variant_type_ptr value_type = parse_variant_type(original_str, i1, end, allow_failure);
			ASSERT_COND(i1 == end, "ERROR PARSING ARRAY TYPE: " << original_str.debug_location());
	
			v.push_back(variant_type_ptr(new variant_type_list(value_type)));
	
			++i1;
		} else {
			ASSERT_COND(false, "UNEXPECTED TOKENS WHEN PARSING TYPE: " << std::string(i1->begin, (i2-1)->end) << " AT " << original_str.debug_location());
		}

		if(i1 != i2 && i1->type == TOKEN_PIPE) {
			++i1;
		} else {
			break;
		}
	}

	if(v.size() == 1) {
		v.front()->set_str(std::string(begin_token->begin, (i1-1)->end));
		return v.front();
	} else {
		variant_type_ptr result(new variant_type_union(v));
		result->set_str(std::string(begin_token->begin, (i1-1)->end));
		return result;
	}
#undef ASSERT_COND
}

variant_type_ptr parse_variant_type(const variant& type)
{
	using namespace formula_tokenizer;
	const std::string& s = type.as_string();
	std::vector<token> tokens;
	std::string::const_iterator i1 = s.begin();
	std::string::const_iterator i2 = s.end();
	while(i1 != i2) {
		try {
			token tok = get_token(i1, i2);
			if(tok.type != TOKEN_WHITESPACE && tok.type != TOKEN_COMMENT) {
				tokens.push_back(tok);
			}
		} catch(token_error& e) {
			ASSERT_LOG(false, "ERROR PARSING TYPE: " << e.msg << " IN '" << s << "' AT " << type.debug_location());
		}
	}

	ASSERT_LOG(tokens.empty() == false, "ERROR PARSING TYPE: EMPTY STRING AT " << type.debug_location());

	const token* begin = &tokens[0];
	return parse_variant_type(type, begin, begin + tokens.size());
}

variant_type_ptr
parse_optional_function_type(const variant& original_str,
                             const formula_tokenizer::token*& i1,
                             const formula_tokenizer::token* i2)
{
	using namespace formula_tokenizer;

	if(i1 == i2 || !i1->equals("def")) {
		return variant_type_ptr();
	}

	++i1;
	if(i1 == i2 || i1->type != TOKEN_LPARENS) {
		return variant_type_ptr();
	}

	int optional_args = 0;

	std::vector<variant_type_ptr> args;

	++i1;
	while(i1 != i2 && i1->type != TOKEN_RPARENS) {
		if(i1->type == TOKEN_IDENTIFIER && i1+1 != i2 &&
		   ((i1+1)->type == TOKEN_COMMA ||
		    (i1+1)->type == TOKEN_RPARENS ||
			(i1+1)->equals("="))) {
			args.push_back(variant_type::get_any());
			++i1;
			if(i1->type == TOKEN_COMMA) {
				++i1;
			} else if(i1->equals("=")) {
				++optional_args;
				while(i1 != i2 && !i1->equals(",") && !i1->equals(")")) {
					++i1;
				}

				if(i1 != i2 && i1->type == TOKEN_COMMA) {
					++i1;
				}
			}
			continue;
		}

		variant_type_ptr arg_type = parse_variant_type(original_str, i1, i2);
		args.push_back(arg_type);
		ASSERT_LOG(i1 != i2, "UNEXPECTED END OF EXPRESSION: " << original_str.debug_location());
		if(i1->type == TOKEN_IDENTIFIER) {
			++i1;

			if(i1 != i2 && i1->equals("=")) {
				++optional_args;

				while(i1 != i2 && !i1->equals(",") && !i1->equals(")")) {
					++i1;
				}
			}
		}

		if(i1 != i2 && i1->type == TOKEN_RPARENS) {
			break;
		}

		ASSERT_LOG(i1 != i2 && i1->type == TOKEN_COMMA, "ERROR PARSING FUNCTION SIGNATURE: " << original_str.debug_location());

		++i1;
	}

	ASSERT_LOG(i1 != i2 && i1->type == TOKEN_RPARENS, "UNEXPECTED END OF FUNCTION SIGNATURE: " << original_str.debug_location());

	variant_type_ptr return_type = variant_type::get_any();

	++i1;
	if(i1 != i2 && i1->type == TOKEN_POINTER) {
		++i1;
		return_type = parse_variant_type(original_str, i1, i2);
	}

	return variant_type::get_function_type(args, return_type, optional_args);
}

variant_type_ptr parse_optional_function_type(const variant& type)
{
	using namespace formula_tokenizer;
	const std::string& s = type.as_string();
	std::vector<token> tokens;
	std::string::const_iterator i1 = s.begin();
	std::string::const_iterator i2 = s.end();
	while(i1 != i2) {
		try {
			token tok = get_token(i1, i2);
			if(tok.type != TOKEN_WHITESPACE && tok.type != TOKEN_COMMENT) {
				tokens.push_back(tok);
			}
		} catch(token_error& e) {
			ASSERT_LOG(false, "ERROR PARSING TYPE: " << e.msg << " IN '" << s << "' AT " << type.debug_location());
		}
	}

	ASSERT_LOG(tokens.empty() == false, "ERROR PARSING TYPE: EMPTY STRING AT " << type.debug_location());

	const token* begin = &tokens[0];
	return parse_optional_function_type(type, begin, begin + tokens.size());
}

variant_type_ptr
parse_optional_formula_type(const variant& original_str,
                            const formula_tokenizer::token*& i1,
                            const formula_tokenizer::token* i2)
{
	variant_type_ptr result = parse_variant_type(original_str, i1, i2, true);
	if(i1 != i2 && i1->equals("<-")) {
		return result;
	}

	return variant_type_ptr();
}

variant_type_ptr parse_optional_formula_type(const variant& type)
{
	using namespace formula_tokenizer;
	const std::string& s = type.as_string();
	std::vector<token> tokens;
	std::string::const_iterator i1 = s.begin();
	std::string::const_iterator i2 = s.end();
	while(i1 != i2) {
		try {
			token tok = get_token(i1, i2);
			if(tok.type != TOKEN_WHITESPACE && tok.type != TOKEN_COMMENT) {
				tokens.push_back(tok);
			}
		} catch(token_error& e) {
			ASSERT_LOG(false, "ERROR PARSING TYPE: " << e.msg << " IN '" << s << "' AT " << type.debug_location());
		}
	}

	ASSERT_LOG(tokens.empty() == false, "ERROR PARSING TYPE: EMPTY STRING AT " << type.debug_location());

	const token* begin = &tokens[0];
	return parse_optional_formula_type(type, begin, begin + tokens.size());
}

variant_type_ptr variant_type::get_any()
{
	static const variant_type_ptr result(new variant_type_any);
	return result;
}

variant_type_ptr variant_type::get_commands()
{
	static const variant_type_ptr result(new variant_type_commands);
	return result;
}

variant_type_ptr variant_type::get_type(variant::TYPE type)
{
	return variant_type_ptr(new variant_type_simple(type));
}

variant_type_ptr variant_type::get_union(const std::vector<variant_type_ptr>& elements)
{
	foreach(variant_type_ptr el, elements) {
		if(!el || el->is_any()) {
			return variant_type::get_any();
		}
	}

	foreach(variant_type_ptr el, elements) {
		const std::vector<variant_type_ptr>* items = el->is_union();
		if(items) {
			std::vector<variant_type_ptr> v = elements;
			v.erase(std::find(v.begin(), v.end(), el));
			v.insert(v.end(), items->begin(), items->end());
			return get_union(v);
		}
	}

	std::vector<variant_type_ptr> items;
	foreach(variant_type_ptr el, elements) {
		foreach(variant_type_ptr item, items) {
			if(el->is_equal(*item)) {
				el = variant_type_ptr();
				break;
			}
		}

		if(el) {
			items.push_back(el);
		}
	}

	if(items.size() == 1) {
		return items[0];
	}

	return variant_type_ptr(new variant_type_union(items));
}

variant_type_ptr variant_type::get_list(variant_type_ptr element_type)
{
	return variant_type_ptr(new variant_type_list(element_type));
}

variant_type_ptr variant_type::get_map(variant_type_ptr key_type, variant_type_ptr value_type)
{
	return variant_type_ptr(new variant_type_map(key_type, value_type));
}

variant_type_ptr variant_type::get_class(const std::string& class_name)
{
	return variant_type_ptr(new variant_type_class(class_name));
}

variant_type_ptr variant_type::get_function_type(const std::vector<variant_type_ptr>& arg_types, variant_type_ptr return_type, int min_args)
{
	return variant_type_ptr(new variant_type_function(arg_types, return_type, min_args));
}

variant_type_ptr variant_type::get_null_excluded(variant_type_ptr input)
{
	variant_type_ptr result = input->null_excluded();
	if(result) {
		return result;
	} else {
		return input;
	}
}

UNIT_TEST(variant_type) {
#define TYPES_COMPAT(a, b) CHECK_EQ(variant_types_compatible(parse_variant_type(variant(a)), parse_variant_type(variant(b))), true)
#define TYPES_INCOMPAT(a, b) CHECK_EQ(variant_types_compatible(parse_variant_type(variant(a)), parse_variant_type(variant(b))), false)

	TYPES_COMPAT("int|bool", "int");
	TYPES_COMPAT("int|bool|string", "string");
	TYPES_COMPAT("decimal", "int");
	TYPES_COMPAT("list", "[int]");
	TYPES_COMPAT("list", "[int|string]");
	TYPES_COMPAT("list", "[any]");
	TYPES_COMPAT("[any]", "[int|string]");
	TYPES_COMPAT("[any]", "list");
	TYPES_COMPAT("{int|string -> string}", "{int -> string}");
	TYPES_COMPAT("map", "{int -> string}");

	TYPES_INCOMPAT("int", "int|bool");
	TYPES_INCOMPAT("int", "decimal");
	TYPES_INCOMPAT("int", "decimal");
	TYPES_INCOMPAT("[int]", "list");
	TYPES_INCOMPAT("{int -> int}", "map");
	TYPES_INCOMPAT("{int -> int}", "{string -> int}");

#undef TYPES_COMPAT	
}
