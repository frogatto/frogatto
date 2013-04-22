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
#include <boost/bind.hpp>
#include <boost/uuid/sha1.hpp>
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <iostream>
#include <iomanip>
#include <stack>
#include <math.h>

#include "array_callable.hpp"
#include "asserts.hpp"
#include "base64.hpp"
#include "code_editor_dialog.hpp"
#include "compress.hpp"
#include "data_blob.hpp"
#include "dialog.hpp"
#include "debug_console.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_callable_definition.hpp"
#include "formula_callable_utils.hpp"
#include "formula_function.hpp"
#include "formula_function_registry.hpp"
#include "formula_object.hpp"
#include "geometry.hpp"
#include "hex_map.hpp"
#include "string_utils.hpp"
#include "unit_test.hpp"
#include "variant_callable.hpp"
#include "controls.hpp"
#include "pathfinding.hpp"
#include "preferences.hpp"
#include "random.hpp"
#include "level.hpp"
#include "json_parser.hpp"
#include "variant_utils.hpp"

#include "graphics.hpp"
#include "module.hpp"
#include <boost/regex.hpp>
#if defined(_WINDOWS)
#include <boost/math/special_functions/asinh.hpp>
#include <boost/math/special_functions/acosh.hpp>
#include <boost/math/special_functions/atanh.hpp>
#define asinh boost::math::asinh
#define acosh boost::math::acosh
#define atanh boost::math::atanh
#endif

#include "compat.hpp"

namespace {
	const std::string FunctionModule = "core";

	const float radians_to_degrees = 57.29577951308232087;
	const std::string EmptyStr;
}

namespace game_logic {

formula_expression::formula_expression(const char* name) : name_(name), begin_str_(EmptyStr.begin()), end_str_(EmptyStr.end()), ntimes_called_(0)
{}

void formula_expression::copy_debug_info_from(const formula_expression& o)
{
	set_debug_info(o.parent_formula_, o.begin_str_, o.end_str_);
}

void formula_expression::set_debug_info(const variant& parent_formula,
                                        std::string::const_iterator begin_str,
                                        std::string::const_iterator end_str)
{
	parent_formula_ = parent_formula;
	begin_str_ = begin_str;
	end_str_ = end_str;
	str_ = std::string(begin_str, end_str);
}

bool formula_expression::has_debug_info() const
{
	return parent_formula_.is_string() && parent_formula_.get_debug_info();
}

const_formula_callable_definition_ptr formula_expression::get_type_definition() const
{
	variant_type_ptr type = query_variant_type();
	if(type) {
		std::string class_name = variant_type_is_class_or_null(type);
		if(class_name.empty() == false) {
			return get_class_definition(class_name);
		}
	}

	return NULL;
}

std::string pinpoint_location(variant v, std::string::const_iterator begin)
{
	return pinpoint_location(v, begin, begin);
}

std::string pinpoint_location(variant v, std::string::const_iterator begin,
                                         std::string::const_iterator end)
{
	std::string str(begin, end);
	if(!v.is_string() || !v.get_debug_info()) {
		return "Unknown location (" + str + ")\n";
	}

	int line_num = v.get_debug_info()->line;

	std::string::const_iterator begin_line = v.as_string().begin();
	while(std::find(begin_line, begin, '\n') != begin) {
		begin_line = std::find(begin_line, begin, '\n')+1;
		++line_num;
	}

	while(begin_line != begin && util::c_isspace(*begin_line)) {
		++begin_line;
	}

	std::string::const_iterator end_line = std::find(begin_line, v.as_string().end(), '\n');

	std::string line(begin_line, end_line);
	int pos = begin - begin_line;

	if(pos > 40) {
		line.erase(line.begin(), line.begin() + pos - 40);
		pos = 40;
		std::fill(line.begin(), line.begin() + 3, '.');
	}

	if(line.size() > 78) {
		line.resize(78);
		std::fill(line.end() - 3, line.end(), '.');
	}

	std::ostringstream s;
	s << "At " << *v.get_debug_info()->filename << " " << line_num << ":\n";
	s << line << "\n";
	for(int n = 0; n != pos; ++n) {
		s << " ";
	}
	s << "^";

	if(end > begin && pos + (end - begin) < line.size()) {
		for(int n = 0; n < (end - begin)-1; ++n) {
			s << "-";
		}
		s << "^";
	}

	s << "\n";
	return s.str();
}

std::string formula_expression::debug_pinpoint_location() const
{
	if(!has_debug_info()) {
		return "Unknown Location (" + str_ + ")\n";
	}

	return pinpoint_location(parent_formula_, begin_str_, end_str_);
}

variant formula_expression::execute_member(const formula_callable& variables, std::string& id, variant* variant_id) const
{
	formula::fail_if_static_context();
	ASSERT_LOG(false, "Trying to set illegal value: " << str_ << "\n" << debug_pinpoint_location());
	return variant();
}

namespace {

variant split_variant_if_str(const variant& s)
{
	if(!s.is_string()) {
		return s;
	}

	std::vector<std::string> v = util::split(s.as_string(), "");
	std::vector<variant> res;
	res.reserve(v.size());
	foreach(const std::string& str, v) {
		res.push_back(variant(str));
	}

	return variant(&res);
}

class ffl_cache : public formula_callable
{
public:
	explicit ffl_cache(int max_entries) : max_entries_(max_entries)
	{}
	const variant* get(const variant& key) const {
		std::map<variant, variant>::const_iterator i = cache_.find(key);
		if(i != cache_.end()) {
			return &i->second;
		} else {
			return NULL;
		}
	}

	void store(const variant& key, const variant& value) const {
		if(cache_.size() == max_entries_) {
			cache_.clear();
		}

		cache_[key] = value;
	}
private:
	variant get_value(const std::string& key) const {
		return variant();
	}

	mutable std::map<variant, variant> cache_;
	int max_entries_;
};

FUNCTION_DEF(overload, 1, -1, "overload(fn...): makes an overload of functions")
	std::vector<variant> functions;
	foreach(expression_ptr expression, args()) {
		functions.push_back(expression->evaluate(variables));
		ASSERT_LOG(functions.back().is_function(), "CALL TO overload() WITH NON-FUNCTION VALUE " << functions.back().write_json());
	}

	return variant::create_function_overload(functions);

FUNCTION_TYPE_DEF
	int min_args = -1;
	std::vector<std::vector<variant_type_ptr> > arg_types;
	std::vector<variant_type_ptr> return_types;
	for(int n = 0; n != args().size(); ++n) {
		variant_type_ptr t = args()[n]->query_variant_type();
		std::vector<variant_type_ptr> a;
		variant_type_ptr return_type;
		int nargs = -1;
		if(t->is_function(&a, &return_type, &nargs) == false) {
			ASSERT_LOG(false, "CALL to overload() with non-function type: " << args()[n]->debug_pinpoint_location());
		}

		return_types.push_back(return_type);
		if(min_args == -1 || nargs < min_args) {
			min_args = nargs;
		}

		for(int m = 0; m != a.size(); ++m) {
			if(arg_types.size() <= m) {
				arg_types.resize(m+1);
			}

			arg_types[m].push_back(a[m]);
		}
	}

	if(min_args < 0) {
		min_args = 0;
	}

	variant_type_ptr return_union = variant_type::get_union(return_types);
	std::vector<variant_type_ptr> arg_union;
	for(int n = 0; n != arg_types.size(); ++n) {
		arg_union.push_back(variant_type::get_union(arg_types[n]));
	}

	return variant_type::get_function_type(arg_union, return_union, min_args);
END_FUNCTION_DEF(overload)

FUNCTION_DEF(create_cache, 0, 1, "create_cache(max_entries=4096): makes an FFL cache object")
	formula::fail_if_static_context();
	int max_entries = 4096;
	if(args().size() >= 1) {
		max_entries = args()[0]->evaluate(variables).as_int();
	}
	return variant(new ffl_cache(max_entries));
END_FUNCTION_DEF(create_cache)

FUNCTION_DEF(query_cache, 3, 3, "query_cache(ffl_cache, key, expr): ")
	const variant key = args()[1]->evaluate(variables);

	const ffl_cache* cache = args()[0]->evaluate(variables).try_convert<ffl_cache>();
	ASSERT_LOG(cache != NULL, "ILLEGAL CACHE ARGUMENT TO query_cache");
	
	const variant* result = cache->get(key);
	if(result != NULL) {
		return *result;
	}

	const variant value = args()[2]->evaluate(variables);
	cache->store(key, value);
	return value;
	
END_FUNCTION_DEF(query_cache)

	class if_function : public function_expression {
	public:
		explicit if_function(const args_list& args)
			: function_expression("if", args, 2, 3)
		{}

		expression_ptr optimize() const {
			variant v;
			if(args()[0]->can_reduce_to_variant(v)) {
				if(v.as_bool()) {
					return args()[1];
				} else {
					if(args().size() == 3) {
						return args()[2];
					} else {
						return expression_ptr(new variant_expression(variant()));
					}
				}
			}

			return expression_ptr();
		}

	private:
		variant execute(const formula_callable& variables) const {
			const int i = args()[0]->evaluate(variables).as_bool() ? 1 : 2;
			if(i >= args().size()) {
				return variant();
			}
			return args()[i]->evaluate(variables);
		}


		variant_type_ptr get_variant_type() const {
			std::vector<variant_type_ptr> types;
			types.push_back(args()[1]->query_variant_type());
			if(args().size() > 2) {
				types.push_back(args()[2]->query_variant_type());
			} else {
				types.push_back(variant_type::get_type(variant::VARIANT_TYPE_NULL));
			}

			return variant_type::get_union(types);
		}

	};

class bound_command : public game_logic::command_callable
{
public:
	bound_command(variant target, const std::vector<variant>& args)
	  : target_(target), args_(args)
	{}
	virtual void execute(game_logic::formula_callable& ob) const {
		ob.execute_command(target_(args_));
	}
private:
	variant target_;
	std::vector<variant> args_;
};

FUNCTION_DEF(bind, 1, -1, "bind(fn, args...)")
	variant fn = args()[0]->evaluate(variables);

	std::vector<variant> arg_values;
	for(int n = 1; n != args().size(); ++n) {
		arg_values.push_back(args()[n]->evaluate(variables));
	}

	return fn.bind_args(arg_values);
FUNCTION_TYPE_DEF
	variant_type_ptr type = args()[0]->query_variant_type();

	std::vector<variant_type_ptr> fn_args;
	variant_type_ptr return_type;
	int min_args = 0;

	if(type->is_function(&fn_args, &return_type, &min_args)) {
		const int nargs = args().size()-1;
		min_args = std::max<int>(0, min_args - nargs);
		if(fn_args.size() <= nargs) {
			fn_args.erase(fn_args.begin(), fn_args.begin() + nargs);
		} else {
			ASSERT_LOG(false, "bind called with too many arguments");
		}

		return variant_type::get_function_type(fn_args, return_type, min_args);
	} else {
		return variant_type::get_type(variant::VARIANT_TYPE_FUNCTION);
	}
END_FUNCTION_DEF(bind)

FUNCTION_DEF(bind_command, 1, -1, "bind_command(fn, args..)")
	variant fn = args()[0]->evaluate(variables);
	if(fn.type() != variant::VARIANT_TYPE_MULTI_FUNCTION) {
		fn.must_be(variant::VARIANT_TYPE_FUNCTION);
	}
	std::vector<variant> args_list;
	for(int n = 1; n != args().size(); ++n) {
		args_list.push_back(args()[n]->evaluate(variables));
	}

	std::string message;
	ASSERT_LOG(fn.function_call_valid(args_list, &message), "Error in bind_command: functions args do not match: " << message);
	
	return variant(new bound_command(fn, args_list));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_CALLABLE);
END_FUNCTION_DEF(bind_command)

FUNCTION_DEF(bind_closure, 2, 2, "bind_closure(fn, obj): binds the given lambda fn to the given object closure")
	variant fn = args()[0]->evaluate(variables);
	return fn.bind_closure(args()[1]->evaluate(variables).as_callable());

END_FUNCTION_DEF(bind_closure)

FUNCTION_DEF(singleton, 1, 1, "singleton(string typename): create a singleton object with the given typename")
	variant type = args()[0]->evaluate(variables);

	static std::map<variant, boost::intrusive_ptr<formula_object> > cache;
	if(cache.count(type)) {
		return variant(cache[type].get());
	}

	boost::intrusive_ptr<formula_object> obj(formula_object::create(type.as_string(), variant()));
	cache[type] = obj;
	return variant(obj.get());
FUNCTION_TYPE_DEF
	variant literal;
	args()[0]->is_literal(literal);
	if(literal.is_string()) {
		std::cerr << "SINGLETON RETURNING CLASS: " << literal.as_string() << "\n";
		return variant_type::get_class(literal.as_string());
	} else {
		std::cerr << "SINGLETON RETURNING ANY: " << literal.write_json() << "\n";
		return variant_type::get_any();
	}
END_FUNCTION_DEF(singleton)

FUNCTION_DEF(construct, 1, 2, "construct(string typename, arg): construct an object with the given typename")
	formula::fail_if_static_context();
	variant type = args()[0]->evaluate(variables);
	variant arg;
	if(args().size() >= 2) {
		arg = args()[1]->evaluate(variables);
	}

	boost::intrusive_ptr<formula_object> obj(formula_object::create(type.as_string(), arg));
	return variant(obj.get());
FUNCTION_TYPE_DEF
	variant literal;
	args()[0]->is_literal(literal);
	if(literal.is_string()) {
		return variant_type::get_class(literal.as_string());
	} else {
		return variant_type::get_any();
	}
END_FUNCTION_DEF(construct)

FUNCTION_DEF(delay_until_end_of_loading, 1, 1, "delay_until_end_of_loading(string): delays evaluation of the enclosed until loading is finished")
	formula::fail_if_static_context();
	variant s = args()[0]->evaluate(variables);
	const_formula_ptr f(formula::create_optional_formula(s));
	if(!f) {
		return variant();
	}

	const_formula_callable_ptr callable(&variables);

	return variant::create_delayed(f, callable);
END_FUNCTION_DEF(delay_until_end_of_loading)

FUNCTION_DEF(eval_no_recover, 1, 2, "eval_no_recover(str, [arg]): evaluate the given string as FFL")
	const_formula_callable_ptr callable(&variables);

	if(args().size() > 1) {
		const variant v = args()[1]->evaluate(variables);
		if(v.is_map()) {
			callable = map_into_callable(v);
		} else {
			callable.reset(v.try_convert<const formula_callable>());
			ASSERT_LOG(callable.get() != NULL, "COULD NOT CONVERT TO CALLABLE: " << v.string_cast());
		}
	}

	variant s = args()[0]->evaluate(variables);

	static std::map<std::string, const_formula_ptr> cache;
	const_formula_ptr& f = cache[s.as_string()];
	if(!f) {
		f = const_formula_ptr(formula::create_optional_formula(s));
	}

	ASSERT_LOG(f.get() != NULL, "ILLEGAL FORMULA GIVEN TO eval: " << s.as_string());
	return f->execute(*callable);
END_FUNCTION_DEF(eval_no_recover)

FUNCTION_DEF(eval, 1, 2, "eval(str, [arg]): evaluate the given string as FFL")
	const_formula_callable_ptr callable(&variables);

	if(args().size() > 1) {
		const variant v = args()[1]->evaluate(variables);
		if(v.is_map()) {
			callable = map_into_callable(v);
		} else {
			callable.reset(v.try_convert<const formula_callable>());
			ASSERT_LOG(callable.get() != NULL, "COULD NOT CONVERT TO CALLABLE: " << v.string_cast());
		}
	}

	variant s = args()[0]->evaluate(variables);
	try {
		static std::map<std::string, const_formula_ptr> cache;
		const assert_recover_scope recovery_scope;

		const_formula_ptr& f = cache[s.as_string()];
		if(!f) {
			f = const_formula_ptr(formula::create_optional_formula(s));
		}

		if(!f) {
			return variant();
		}

		return f->execute(*callable);
	} catch(type_error&) {
	} catch(validation_failure_exception&) {
	}
	std::cerr << "ERROR IN EVAL\n";
	return variant();
END_FUNCTION_DEF(eval)

FUNCTION_DEF(handle_errors, 2, 2, "handle_errors(expr, failsafe): evaluates 'expr' and returns it. If expr has fatal errors in evaluation, return failsafe instead. 'failsafe' is an expression which receives 'error_msg' and 'context' as parameters.")
	const assert_recover_scope recovery_scope;
	try {
		return args()[0]->evaluate(variables);
	} catch(validation_failure_exception& e) {
		boost::intrusive_ptr<map_formula_callable> callable(new map_formula_callable(&variables));
		callable->add("context", variant(&variables));
		callable->add("error_msg", variant(e.msg));
		return args()[1]->evaluate(*callable);
	}
FUNCTION_TYPE_DEF
	return args()[0]->query_variant_type();
END_FUNCTION_DEF(handle_errors)

FUNCTION_DEF(switch, 3, -1, "switch(value, case1, result1, case2, result2 ... casen, resultn, default) -> value: returns resultn where value = casen, or default otherwise.")
	variant var = args()[0]->evaluate(variables);
	for(size_t n = 1; n < args().size()-1; n += 2) {
		variant val = args()[n]->evaluate(variables);
		if(val == var) {
			return args()[n+1]->evaluate(variables);
		}
	}

	if((args().size()%2) == 0) {
		return args().back()->evaluate(variables);
	} else {
		return variant();
	}
FUNCTION_TYPE_DEF
	std::vector<variant_type_ptr> types;
	for(int n = 1; n < args().size(); ++n) {
		if(n%2 == 1 || n == args().size()-1) {
			types.push_back(args()[n]->query_variant_type());
		}
	}

	return variant_type::get_union(types);
END_FUNCTION_DEF(switch)

FUNCTION_DEF(query, 2, 2, "query(object, str): evaluates object.str")
	variant callable = args()[0]->evaluate(variables);
	variant str = args()[1]->evaluate(variables);
	return callable.as_callable()->query_value(str.as_string());
END_FUNCTION_DEF(query)

FUNCTION_DEF(call, 2, 2, "call(fn, list): calls the given function with 'list' as the arguments")
	variant fn = args()[0]->evaluate(variables);
	variant a = args()[1]->evaluate(variables);
	return fn(a.as_list());
FUNCTION_TYPE_DEF
	variant_type_ptr fn_type = args()[0]->query_variant_type();
	variant_type_ptr return_type;
	if(fn_type->is_function(NULL, &return_type, NULL)) {
		return return_type;
	}

	return variant_type_ptr();
END_FUNCTION_DEF(call)


FUNCTION_DEF(abs, 1, 1, "abs(value) -> value: evaluates the absolute value of the value given")
	variant v = args()[0]->evaluate(variables);
	if(v.is_decimal()) {
		const decimal d = v.as_decimal();
		return variant(d >= 0 ? d : -d);
	} else {
		const int n = v.as_int();
		return variant(n >= 0 ? n : -n);
	}
FUNCTION_TYPE_DEF
	return args()[0]->query_variant_type();
END_FUNCTION_DEF(abs)

FUNCTION_DEF(sign, 1, 1, "sign(value) -> value: evaluates to 1 if positive, -1 if negative, and 0 if 0")
	const int n = args()[0]->evaluate(variables).as_int();
	if(n > 0) {
		return variant(1);
	} else if(n < 0) {
		return variant(-1);
	} else {
		return variant(0);
	}
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_INT);
END_FUNCTION_DEF(sign)

FUNCTION_DEF(median, 1, -1, "median(args...) -> value: evaluates to the median of the given arguments. If given a single argument list, will evaluate to the median of the member items.")
	if(args().size() == 3) {
		//special case for 3 arguments since it's a common case.
		variant a = args()[0]->evaluate(variables);
		variant b = args()[1]->evaluate(variables);
		variant c = args()[2]->evaluate(variables);
		if(a < b) {
			if(b < c) {
				return b;
			} else if(a < c) {
				return c;
			} else {
				return a;
			}
		} else {
			if(a < c) {
				return a;
			} else if(b < c) {
				return c;
			} else {
				return b;
			}
		}
	}

	std::vector<variant> items;
	if(args().size() != 1) {
		items.reserve(args().size());
	}

	for(size_t n = 0; n != args().size(); ++n) {
		const variant v = args()[n]->evaluate(variables);
		if(args().size() == 1 && v.is_list()) {
			items = v.as_list();
		} else {
			items.push_back(v);
		}
	}

	std::sort(items.begin(), items.end());
	if(items.empty()) {
		return variant();
	} else if(items.size()&1) {
		return items[items.size()/2];
	} else {
		return (items[items.size()/2-1] + items[items.size()/2])/variant(2);
	}
FUNCTION_TYPE_DEF
	return args()[0]->query_variant_type()->is_list_of();
END_FUNCTION_DEF(median)

FUNCTION_DEF(min, 1, -1, "min(args...) -> value: evaluates to the minimum of the given arguments. If given a single argument list, will evaluate to the minimum of the member items.")

	bool found = false;
	variant res;
	for(size_t n = 0; n != args().size(); ++n) {
		const variant v = args()[n]->evaluate(variables);
		if(v.is_list() && args().size() == 1) {
			for(size_t m = 0; m != v.num_elements(); ++m) {
				if(!found || v[m] < res) {
					res = v[m];
					found = true;
				}
			}
		} else {
			if(!found || v < res) {
				res = v;
				found = true;
			}
		}
	}

	return res;
FUNCTION_TYPE_DEF
	if(args().size() == 1) {
		return args()[0]->query_variant_type()->is_list_of();
	} else {
		std::vector<variant_type_ptr> types;
		for(int n = 0; n != args().size(); ++n) {
			types.push_back(args()[n]->query_variant_type());
		}

		return variant_type::get_union(types);
	}
END_FUNCTION_DEF(min)

FUNCTION_DEF(max, 1, -1, "max(args...) -> value: evaluates to the maximum of the given arguments. If given a single argument list, will evaluate to the maximum of the member items.")

	bool found = false;
	variant res;
	for(size_t n = 0; n != args().size(); ++n) {
		const variant v = args()[n]->evaluate(variables);
		if(v.is_list() && args().size() == 1) {
			for(size_t m = 0; m != v.num_elements(); ++m) {
				if(!found || v[m] > res) {
					res = v[m];
					found = true;
				}
			}
		} else {
			if(!found || v > res) {
				res = v;
				found = true;
			}
		}
	}

	return res;
FUNCTION_TYPE_DEF
	if(args().size() == 1) {
		return args()[0]->query_variant_type()->is_list_of();
	} else {
		std::vector<variant_type_ptr> types;
		for(int n = 0; n != args().size(); ++n) {
			types.push_back(args()[n]->query_variant_type());
		}

		return variant_type::get_union(types);
	}
END_FUNCTION_DEF(max)

	UNIT_TEST(min_max_decimal) {
		CHECK(game_logic::formula(variant("max(1,1.4)")).execute() == game_logic::formula(variant("1.4")).execute(), "test failed");
	}

FUNCTION_DEF(keys, 1, 1, "keys(map) -> list: gives the keys for a map")
	const variant map = args()[0]->evaluate(variables);
	if(map.is_callable()) {
		std::vector<variant> v;
		const std::vector<formula_input> inputs = map.as_callable()->inputs();
		foreach(const formula_input& in, inputs) {
			v.push_back(variant(in.name));
		}

		return variant(&v);
	}

	return map.get_keys();
FUNCTION_TYPE_DEF
	return variant_type::get_list(args()[0]->query_variant_type()->is_map_of().first);
END_FUNCTION_DEF(keys)

FUNCTION_DEF(values, 1, 1, "values(map) -> list: gives the values for a map")
	const variant map = args()[0]->evaluate(variables);
	return map.get_values();
FUNCTION_TYPE_DEF
	return args()[0]->query_variant_type()->is_map_of().second;
END_FUNCTION_DEF(values)

FUNCTION_DEF(wave, 1, 1, "wave(int) -> int: a wave with a period of 1000 and height of 1000")
	const int value = args()[0]->evaluate(variables).as_int()%1000;
	const double angle = 2.0*3.141592653589*(static_cast<double>(value)/1000.0);
	return variant(static_cast<int>(sin(angle)*1000.0));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_INT);
END_FUNCTION_DEF(wave)

FUNCTION_DEF(decimal, 1, 1, "decimal(value) -> decimal: converts the value to a decimal")
	return variant(args()[0]->evaluate(variables).as_decimal());
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(decimal)

FUNCTION_DEF(integer, 1, 1, "integer(value) -> int: converts the value to an integer")
	return variant(args()[0]->evaluate(variables).as_int());
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_INT);
END_FUNCTION_DEF(integer)

FUNCTION_DEF(sin, 1, 1, "sin(x): Standard sine function.")
	const float angle = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(sin(angle/radians_to_degrees)));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(sin)

FUNCTION_DEF(cos, 1, 1, "cos(x): Standard cosine function.")
	const float angle = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(cos(angle/radians_to_degrees)));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(cos)

FUNCTION_DEF(tan, 1, 1, "tan(x): Standard tangent function.")
	const float angle = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(tan(angle/radians_to_degrees)));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(tan)

FUNCTION_DEF(asin, 1, 1, "asin(x): Standard arc sine function.")
	const float ratio = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(asin(ratio)*radians_to_degrees));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(asin)

FUNCTION_DEF(acos, 1, 1, "acos(x): Standard arc cosine function.")
	const float ratio = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(acos(ratio)*radians_to_degrees));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(acos)

FUNCTION_DEF(atan, 1, 1, "atan(x): Standard arc tangent function.")
	const float ratio = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(atan(ratio)*radians_to_degrees));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(atan)

FUNCTION_DEF(sinh, 1, 1, "sinh(x): Standard hyperbolic sine function.")
	const float angle = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(sinh(angle)));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(sinh)

FUNCTION_DEF(cosh, 1, 1, "cosh(x): Standard hyperbolic cosine function.")
	const float angle = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(cosh(angle)));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(cosh)

FUNCTION_DEF(tanh, 1, 1, "tanh(x): Standard hyperbolic tangent function.")
	const float angle = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(tanh(angle)));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(tanh)

FUNCTION_DEF(asinh, 1, 1, "asinh(x): Standard arc hyperbolic sine function.")
	const float ratio = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(asinh(ratio)));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(asinh)

FUNCTION_DEF(acosh, 1, 1, "acosh(x): Standard arc hyperbolic cosine function.")
	const float ratio = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(acosh(ratio)));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(acosh)

FUNCTION_DEF(atanh, 1, 1, "atanh(x): Standard arc hyperbolic tangent function.")
	const float ratio = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(static_cast<decimal>(atanh(ratio)));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(atanh)

FUNCTION_DEF(sqrt, 1, 1, "sqrt(x): Returns the square root of x.")
	const double value = args()[0]->evaluate(variables).as_decimal().as_float();
	return variant(decimal(sqrt(value)));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_DECIMAL);
END_FUNCTION_DEF(sqrt)

FUNCTION_DEF(angle, 4, 4, "angle(x1, y1, x2, y2) -> int: Returns the angle, from 0°, made by the line described by the two points (x1, y1) and (x2, y2).")
	const float a = args()[0]->evaluate(variables).as_int();
	const float b = args()[1]->evaluate(variables).as_int();
	const float c = args()[2]->evaluate(variables).as_int();
	const float d = args()[3]->evaluate(variables).as_int();
	return variant(static_cast<int>(round((atan2(a-c, b-d)*radians_to_degrees+90)*VARIANT_DECIMAL_PRECISION)*-1), variant::DECIMAL_VARIANT);
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_INT);
END_FUNCTION_DEF(angle)

FUNCTION_DEF(angle_delta, 2, 2, "angle_delta(a, b) -> int: Given two angles, returns the smallest rotation needed to make a equal to b.")
	int a = args()[0]->evaluate(variables).as_int();
	int b = args()[1]->evaluate(variables).as_int();
	while(abs(a - b) > 180) {
		if(a < b) {
			a += 360;
		} else {
			b += 360;
		}
	}

	return variant(b - a);
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_INT);
END_FUNCTION_DEF(angle_delta)

FUNCTION_DEF(orbit, 4, 4, "orbit(x, y, angle, dist) -> [x,y]: Returns the point as a list containing an x/y pair which is dist away from the point as defined by x and y passed in, at the angle passed in.")
	const float x = args()[0]->evaluate(variables).as_decimal().as_float();
	const float y = args()[1]->evaluate(variables).as_decimal().as_float();
	const float ang = args()[2]->evaluate(variables).as_decimal().as_float();
	const float dist = args()[3]->evaluate(variables).as_decimal().as_float();
	
	const float u = (dist * cos(ang/radians_to_degrees)) + x;   //TODO Find out why whole number decimals are returned.
	const float v = (dist * sin(ang/radians_to_degrees)) + y;

	std::vector<variant> result;
	result.reserve(2);
	result.push_back(variant(decimal(u)));
	result.push_back(variant(decimal(v)));
	
	return variant(&result);
FUNCTION_TYPE_DEF
	return variant_type::get_list(variant_type::get_type(variant::VARIANT_TYPE_DECIMAL));
END_FUNCTION_DEF(orbit)

FUNCTION_DEF(regex_replace, 3, 3, "regex_replace(string, string, string) -> string: Unknown.")
	const std::string str = args()[0]->evaluate(variables).as_string();
	const boost::regex re(args()[1]->evaluate(variables).as_string());
	const std::string value = args()[2]->evaluate(variables).as_string();
	return variant(boost::regex_replace(str, re, value));
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_STRING);
END_FUNCTION_DEF(regex_replace)

FUNCTION_DEF(regex_match, 2, 2, "regex_match(string, re_string) -> string: returns null if not found, else returns the whole string or a list of sub-strings depending on whether blocks were demarcated.")
	const std::string str = args()[0]->evaluate(variables).as_string();
	const boost::regex re(args()[1]->evaluate(variables).as_string());
	 boost::match_results<std::string::const_iterator> m;
	if(boost::regex_match(str, m, re) == false) {
		return variant();
	}
	if(m.size() == 1) {
		return variant(std::string(m[0].first, m[0].second));
	} 
	std::vector<variant> v;
	for(size_t i = 1; i < m.size(); i++) {
		v.push_back(variant(std::string(m[i].first, m[i].second)));
	}
	return variant(&v);
FUNCTION_TYPE_DEF
	std::vector<variant_type_ptr> types;
	types.push_back(variant_type::get_list(variant_type::get_type(variant::VARIANT_TYPE_STRING)));
	types.push_back(variant_type::get_type(variant::VARIANT_TYPE_STRING));
	types.push_back(variant_type::get_type(variant::VARIANT_TYPE_NULL));
	return variant_type::get_union(types);
END_FUNCTION_DEF(regex_match)

namespace {
class variant_comparator : public formula_callable {
	expression_ptr expr_;
	const formula_callable* fallback_;
	mutable variant a_, b_;
	variant get_value(const std::string& key) const {
		if(key == "a") {
			return a_;
		} else if(key == "b") {
			return b_;
		} else {
			return fallback_->query_value(key);
		}
	}

	variant get_value_by_slot(int slot) const {
		if(slot == 0) {
			return a_;
		} else if(slot == 1) {
			return b_;
		}

		return fallback_->query_value_by_slot(slot - 2);
	}

	void get_inputs(std::vector<formula_input>* inputs) const {
		fallback_->get_inputs(inputs);
	}
public:
	variant_comparator(const expression_ptr& expr, const formula_callable& fallback) : formula_callable(false), expr_(expr), fallback_(&fallback)
	{}

	bool operator()(const variant& a, const variant& b) const {
		a_ = a;
		b_ = b;
		return expr_->evaluate(*this).as_bool();
	}

	variant eval(const variant& a, const variant& b) const {
		a_ = a;
		b_ = b;
		return expr_->evaluate(*this);
	}
};

class variant_comparator_definition : public formula_callable_definition
{
public:
	variant_comparator_definition(const_formula_callable_definition_ptr base, variant_type_ptr type)
	  : base_(base), type_(type)
	{
		for(int n = 0; n != 2; ++n) {
			const std::string name = (n == 0) ? "a" : "b";
			entries_.push_back(entry(name));
			entries_.back().set_variant_type(type_);
		}
	}

	int get_slot(const std::string& key) const {
		if(key == "a") { return 0; }
		if(key == "b") { return 1; }

		if(base_) {
			int result = base_->get_slot(key);
			if(result >= 0) {
				result += 2;
			}

			return result;
		} else {
			return -1;
		}
	}

	entry* get_entry(int slot) {
		if(slot < 0) {
			return NULL;
		}

		if(slot < entries_.size()) {
			return &entries_[slot];
		}

		if(base_) {
			return const_cast<formula_callable_definition*>(base_.get())->get_entry(slot - entries_.size());
		}

		return NULL;
	}

	const entry* get_entry(int slot) const {
		if(slot < 0) {
			return NULL;
		}

		if(slot < entries_.size()) {
			return &entries_[slot];
		}

		if(base_) {
			return base_->get_entry(slot - entries_.size());
		}

		return NULL;
	}

	int num_slots() const {
		return 2 + (base_ ? base_->num_slots() : 0);
	}

private:
	const_formula_callable_definition_ptr base_;
	variant_type_ptr type_;

	std::vector<entry> entries_;
};
}

FUNCTION_DEF(fold, 2, 3, "fold(list, expr, [default]) -> value")
	variant list = args()[0]->evaluate(variables);
	const int size = list.num_elements();
	if(size == 0) {
		if(args().size() >= 3) {
			return args()[2]->evaluate(variables);
		} else {
			return variant();
		}
	} else if(size == 1) {
		return list[0];
	}

	boost::intrusive_ptr<variant_comparator> callable(new variant_comparator(args()[1], variables));

	variant a = list[0];
	for(int n = 1; n < list.num_elements(); ++n) {
		a = callable->eval(a, list[n]);
	}

	return a;
FUNCTION_TYPE_DEF
	std::vector<variant_type_ptr> types;
	types.push_back(args()[1]->query_variant_type());
	if(args().size() > 2) {
		types.push_back(args()[2]->query_variant_type());
	}

	return variant_type::get_union(types);
END_FUNCTION_DEF(fold)

FUNCTION_DEF(unzip, 1, 1, "unzip(list of lists) -> list of lists: Converts [[1,4],[2,5],[3,6]] -> [[1,2,3],[4,5,6]]")
	variant item1 = args()[0]->evaluate(variables);
	ASSERT_LOG(item1.is_list(), "unzip function arguments must be a list");

	// Calculate breadth and depth of new list.
	const int depth = item1.num_elements();
	size_t breadth = 0;
	for(size_t n = 0; n < item1.num_elements(); ++n) {
		ASSERT_LOG(item1[n].is_list(), "Item " << n << " on list isn't list");
		breadth = std::max(item1[n].num_elements(), breadth);
	}

	std::vector<std::vector<variant> > v;
	for(size_t n = 0; n < breadth; ++n) {
		std::vector<variant> e1;
		e1.resize(depth);
		v.push_back(e1);
	}

	for(size_t n = 0; n < item1.num_elements(); ++n) {
		for(size_t m = 0; m < item1[n].num_elements(); ++m) {
			v[m][n] = item1[n][m];
		}
	}

	std::vector<variant> vl;
	for(size_t n = 0; n < v.size(); ++n) {
		vl.push_back(variant(&v[n]));
	}
	return variant(&vl);
END_FUNCTION_DEF(unzip)

FUNCTION_DEF(zip, 3, 3, "zip(list1, list2, expr) -> list")
	const variant item1 = args()[0]->evaluate(variables);
	const variant item2 = args()[1]->evaluate(variables);

	ASSERT_LOG(item1.type() == item2.type(), "zip function arguments must both be the same type.");
	ASSERT_LOG(item1.is_list() || item1.is_map(), "zip function arguments must be either lists or maps");

	boost::intrusive_ptr<variant_comparator> callable(new variant_comparator(args()[2], variables));
	const int size = std::min(item1.num_elements(), item2.num_elements());

	if(item1.is_list()) {
		std::vector<variant> result;
		// is list
		for(int n = 0; n < size; ++n) {
			result.push_back(callable->eval(item1[n], item2[n]));
		}
		return variant(&result);
	} else {
		std::map<variant,variant> retMap(item1.as_map());
		variant keys = item2.get_keys();
		for(int n = 0; n != keys.num_elements(); n++) {
			if(retMap[keys[n]].is_null() == false) {
				retMap[keys[n]] = callable->eval(retMap[keys[n]], item2[keys[n]]);
			} else {
				retMap[keys[n]] = item2[keys[n]];
			}
		}
		return variant(&retMap);
	}
	return variant();
END_FUNCTION_DEF(zip)

FUNCTION_DEF(float_array, 1, 2, "float_array(list, (opt) num_elements) -> callable: Converts a list of floating point values into an efficiently accessible object.")
	game_logic::formula::fail_if_static_context();
	variant f = args()[0]->evaluate(variables);
	int num_elems = args().size() == 1 ? 1 : args()[1]->evaluate(variables).as_int();
	std::vector<GLfloat> floats;
	for(size_t n = 0; n < f.num_elements(); ++n) {
		floats.push_back(GLfloat(f[n].as_decimal().as_float()));
	}
	return variant(new float_array_callable(&floats, num_elems));
END_FUNCTION_DEF(float_array)

FUNCTION_DEF(short_array, 1, 2, "short_array(list) -> callable: Converts a list of integer values into an efficiently accessible object.")
	game_logic::formula::fail_if_static_context();
	variant s = args()[0]->evaluate(variables);
	int num_elems = args().size() == 1 ? 1 : args()[1]->evaluate(variables).as_int();
	std::vector<GLshort> shorts;
	for(size_t n = 0; n < s.num_elements(); ++n) {
		shorts.push_back(GLshort(s[n].as_int()));
	}
	return variant(new short_array_callable(&shorts, num_elems));
END_FUNCTION_DEF(short_array)

/* XXX Krista to be reworked
FUNCTION_DEF(update_controls, 1, 1, "update_controls(map) : Updates the controls based on a list of id:string, pressed:bool pairs")
	const variant map = args()[0]->evaluate(variables);
	foreach(const variant_pair& p, map.as_map()) {
		std::cerr << "Button: " << p.first.as_string() << " " << (p.second.as_bool() ? "Pressed" : "Released") << std::endl;
		controls::update_control_state(p.first.as_string(), p.second.as_bool());
	}
	return variant();
END_FUNCTION_DEF(update_controls)

FUNCTION_DEF(map_controls, 1, 1, "map_controls(map) : Creates or updates the mapping on controls to keys")
	const variant map = args()[0]->evaluate(variables);
	foreach(const variant_pair& p, map.as_map()) {
		controls::set_mapped_key(p.first.as_string(), static_cast<SDL_Keycode>(p.second.as_int()));
	}
	return variant();
END_FUNCTION_DEF(map_controls)*/

FUNCTION_DEF(directed_graph, 2, 2, "directed_graph(list_of_vertexes, adjacent_expression) -> a directed graph")
	variant vertices = args()[0]->evaluate(variables);
	pathfinding::graph_edge_list edges;
	
	std::vector<variant> vertex_list;
	boost::intrusive_ptr<map_formula_callable> callable(new map_formula_callable(&variables));
	variant& a = callable->add_direct_access("v");
	foreach(variant v, vertices.as_list()) {
		a = v;
		edges[v] = args()[1]->evaluate(*callable).as_list();
		vertex_list.push_back(v);
	}
	pathfinding::directed_graph* dg = new pathfinding::directed_graph(&vertex_list, &edges);
	return variant(dg);
END_FUNCTION_DEF(directed_graph)

FUNCTION_DEF(weighted_graph, 2, 2, "weighted_graph(directed_graph, weight_expression) -> a weighted directed graph")
	variant graph = args()[0]->evaluate(variables);
	pathfinding::directed_graph_ptr dg = boost::intrusive_ptr<pathfinding::directed_graph>(graph.try_convert<pathfinding::directed_graph>());
	ASSERT_LOG(dg, "Directed graph given is not of the correct type.");
	pathfinding::edge_weights w;
	boost::intrusive_ptr<map_formula_callable> callable(new map_formula_callable(&variables));
	variant& a = callable->add_direct_access("a");
	variant& b = callable->add_direct_access("b");
	for(pathfinding::graph_edge_list::const_iterator edges = dg->get_edges()->begin(); 
		edges != dg->get_edges()->end(); 
		edges++) {
		foreach(const variant e2, edges->second) {
			a = edges->first;
			b = e2;
			w[pathfinding::graph_edge(edges->first, e2)] = args()[1]->evaluate(*callable).as_decimal();
		}
	}
	return variant(new pathfinding::weighted_directed_graph(dg, &w));
END_FUNCTION_DEF(weighted_graph)

FUNCTION_DEF(a_star_search, 4, 4, "a_star_search(weighted_directed_graph, src_node, dst_node, heuristic) -> A list of nodes which represents the 'best' path from src_node to dst_node.")
	variant graph = args()[0]->evaluate(variables);
	pathfinding::weighted_directed_graph_ptr wg = graph.try_convert<pathfinding::weighted_directed_graph>();
	ASSERT_LOG(wg, "Weighted graph given is not of the correct type.");
	variant src_node = args()[1]->evaluate(variables);
	variant dst_node = args()[2]->evaluate(variables);
	expression_ptr heuristic = args()[3];
	boost::intrusive_ptr<map_formula_callable> callable(new map_formula_callable(&variables));
	return pathfinding::a_star_search(wg, src_node, dst_node, heuristic, callable);
END_FUNCTION_DEF(a_star_search)

FUNCTION_DEF(path_cost_search, 3, 3, "cost_search(weighted_directed_graph, src_node, max_cost) -> A list of all possible points reachable from src_node within max_cost.")
	variant graph = args()[0]->evaluate(variables);
	pathfinding::weighted_directed_graph_ptr wg = graph.try_convert<pathfinding::weighted_directed_graph>();
	ASSERT_LOG(wg, "Weighted graph given is not of the correct type.");
	variant src_node = args()[1]->evaluate(variables);
	decimal max_cost(args()[2]->evaluate(variables).as_decimal());
	return pathfinding::path_cost_search(wg, src_node, max_cost);
END_FUNCTION_DEF(path_cost_search)

FUNCTION_DEF(create_graph_from_level, 1, 3, "create_graph_from_level(level, (optional) tile_size_x, (optional) tile_size_y) -> directed graph : Creates a directed graph based on the current level.")
	int tile_size_x = TileSize;
	int tile_size_y = TileSize;
	if(args().size() == 2) {
		tile_size_y = tile_size_x = args()[1]->evaluate(variables).as_int();
	} else if(args().size() == 3) {
		tile_size_x = args()[1]->evaluate(variables).as_int();
		tile_size_y = args()[2]->evaluate(variables).as_int();
	}
	ASSERT_LOG((tile_size_x%2)==0 && (tile_size_y%2)==0, "The tile_size_x and tile_size_y values *must* be even. (" << tile_size_x << "," << tile_size_y << ")");
	variant curlevel = args()[0]->evaluate(variables);
	level_ptr lvl = curlevel.try_convert<level>();
	ASSERT_LOG(lvl, "The level parameter passed to the function was couldn't be converted.");
	rect b = lvl->boundaries();
	b.from_coordinates(b.x() - b.x()%tile_size_x, 
		b.y() - b.y()%tile_size_y, 
		b.x2()+(tile_size_x-b.x2()%tile_size_x), 
		b.y2()+(tile_size_y-b.y2()%tile_size_y));

	pathfinding::graph_edge_list edges;
	std::vector<variant> vertex_list;
	const rect& b_rect = level::current().boundaries();

	for(int y = b.y(); y < b.y2(); y += tile_size_y) {
		for(int x = b.x(); x < b.x2(); x += tile_size_x) {
			if(!lvl->solid(x, y, tile_size_x, tile_size_y)) {
				variant l(pathfinding::point_as_variant_list(point(x,y)));
				vertex_list.push_back(l);
				std::vector<variant> e;
				point po(x,y);
				foreach(const point& p, pathfinding::get_neighbours_from_rect(po, tile_size_x, tile_size_y, b_rect)) {
					if(!lvl->solid(p.x, p.y, tile_size_x, tile_size_y)) {
						e.push_back(pathfinding::point_as_variant_list(p));
					}
				}
				edges[l] = e;
			}
		}
	}
	return variant(new pathfinding::directed_graph(&vertex_list, &edges));
END_FUNCTION_DEF(create_graph_from_level)

FUNCTION_DEF(plot_path, 6, 9, "plot_path(level, from_x, from_y, to_x, to_y, heuristic, (optional) weight_expr, (optional) tile_size_x, (optional) tile_size_y) -> list : Returns a list of points to get from (from_x, from_y) to (to_x, to_y)")
	int tile_size_x = TileSize;
	int tile_size_y = TileSize;
	expression_ptr weight_expr = expression_ptr();
	variant curlevel = args()[0]->evaluate(variables);
	level_ptr lvl = curlevel.try_convert<level>();
	if(args().size() > 6) {
		weight_expr = args()[6];
	}
	if(args().size() == 8) {
		tile_size_y = tile_size_x = args()[6]->evaluate(variables).as_int();
	} else if(args().size() == 9) {
		tile_size_x = args()[6]->evaluate(variables).as_int();
		tile_size_y = args()[7]->evaluate(variables).as_int();
	}
	ASSERT_LOG((tile_size_x%2)==0 && (tile_size_y%2)==0, "The tile_size_x and tile_size_y values *must* be even. (" << tile_size_x << "," << tile_size_y << ")");
	point src(args()[1]->evaluate(variables).as_int(), args()[2]->evaluate(variables).as_int());
	point dst(args()[3]->evaluate(variables).as_int(), args()[4]->evaluate(variables).as_int());
	expression_ptr heuristic = args()[4];
	boost::intrusive_ptr<map_formula_callable> callable(new map_formula_callable(&variables));
	return variant(pathfinding::a_star_find_path(lvl, src, dst, heuristic, weight_expr, callable, tile_size_x, tile_size_y));
END_FUNCTION_DEF(plot_path)

FUNCTION_DEF(sort, 1, 2, "sort(list, criteria): Returns a nicely-ordered list. If you give it an optional formula such as 'a>b' it will sort it according to that. This example favours larger numbers first instead of the default of smaller numbers first.")
	variant list = args()[0]->evaluate(variables);
	std::vector<variant> vars;
	vars.reserve(list.num_elements());
	for(size_t n = 0; n != list.num_elements(); ++n) {
		vars.push_back(list[n]);
	}

	if(args().size() == 1) {
		std::sort(vars.begin(), vars.end());
	} else {
		boost::intrusive_ptr<variant_comparator> comparator(new variant_comparator(args()[1], variables));
		std::sort(vars.begin(), vars.end(), *comparator);
	}

	return variant(&vars);
FUNCTION_TYPE_DEF
	return args()[0]->query_variant_type();
END_FUNCTION_DEF(sort)

namespace {
//our own shuffle, to guarantee consistency across different machines.
template<typename RnIt>
void myshuffle(RnIt i1, RnIt i2)
{
	while(i2 - i1 > 1) {
		std::swap(*(i2-1), i1[rng::generate()%(i2-i1)]);
		--i2;
	}
}
}

FUNCTION_DEF(shuffle, 1, 1, "shuffle(list) - Returns a shuffled version of the list. Like shuffling cards.")
	variant list = args()[0]->evaluate(variables);
	boost::intrusive_ptr<float_array_callable> f = list.try_convert<float_array_callable>();
	if(f != NULL) {
		std::vector<GLfloat> floats(f->floats().begin(), f->floats().end());
		myshuffle(floats.begin(), floats.end());
		return variant(new float_array_callable(&floats));
	}
	
	boost::intrusive_ptr<short_array_callable> s = list.try_convert<short_array_callable>();
	if(s != NULL) {
		std::vector<GLshort> shorts(s->shorts().begin(), s->shorts().end());
		myshuffle(shorts.begin(), shorts.end());
		return variant(new short_array_callable(&shorts));
	}

	std::vector<variant> vars;
	vars.reserve(list.num_elements());
	for(size_t n = 0; n != list.num_elements(); ++n) {
		vars.push_back(list[n]);
	}

	myshuffle(vars.begin(), vars.end());

	return variant(&vars);
FUNCTION_TYPE_DEF
	return args()[0]->query_variant_type();
END_FUNCTION_DEF(shuffle)

FUNCTION_DEF(remove_from_map, 2, 2, "remove_from_map(map, key): Removes the given key from the map and returns it.")
	variant m = args()[0]->evaluate(variables);
	ASSERT_LOG(m.is_map(), "ARG PASSED TO remove_from_map() IS NOT A MAP");
	variant key = args()[1]->evaluate(variables);
	return m.remove_attr(key);
FUNCTION_TYPE_DEF
	return args()[0]->query_variant_type();
END_FUNCTION_DEF(remove_from_map)
	
namespace {
	void flatten_items( variant items, std::vector<variant>* output){
		for(size_t n = 0; n != items.num_elements(); ++n) {
			
			if( items[n].is_list() ){
				flatten_items(items[n], output);
			} else {
				output->push_back(items[n]);
			}
			
		}
	}
	
}

FUNCTION_DEF(flatten, 1, 1, "flatten(list): Returns a list with a depth of 1 containing the elements of any list passed in.")
	variant input = args()[0]->evaluate(variables);
	std::vector<variant> output;
	flatten_items(input, &output);
	return variant(&output);
END_FUNCTION_DEF(flatten)

enum MAP_CALLABLE_SLOT { MAP_CALLABLE_VALUE, MAP_CALLABLE_INDEX, MAP_CALLABLE_CONTEXT, MAP_CALLABLE_KEY, NUM_MAP_CALLABLE_SLOTS };
static const std::string MapCallableFields[] = { "value", "index", "context", "key" };

class map_callable_definition : public formula_callable_definition
{
public:
	map_callable_definition(const_formula_callable_definition_ptr base, variant_type_ptr key_type, variant_type_ptr value_type, const std::string& value_name)
	  : base_(base), key_type_(key_type), value_type_(value_type)
	{
		for(int n = 0; n != NUM_MAP_CALLABLE_SLOTS; ++n) {
			entries_.push_back(entry(MapCallableFields[n]));
			std::string class_name;
			switch(n) {
			case MAP_CALLABLE_VALUE:
				if(!value_name.empty()) {
					entries_.back().id = value_name;
				}

				if(value_type_) {
					entries_.back().variant_type = value_type_;
					if(entries_.back().variant_type->is_class(&class_name)) {
						entries_.back().type_definition = get_class_definition(class_name);
					}
				}
				break;
			case MAP_CALLABLE_INDEX:
				entries_.back().variant_type = variant_type::get_type(variant::VARIANT_TYPE_INT);
				break;
			case MAP_CALLABLE_CONTEXT:
				entries_.back().variant_type = value_type_;
				entries_.back().type_definition = base;
				break;
			case MAP_CALLABLE_KEY:
				if(key_type_) {
					entries_.back().variant_type = key_type_;
					if(key_type_->is_class(&class_name)) {
						entries_.back().type_definition = get_class_definition(class_name);
					}
				}
				break;
			}
		}
	}

	int get_slot(const std::string& key) const {
		for(int n = 0; n != entries_.size(); ++n) {
			if(entries_[n].id == key) {
				return n;
			}
		}

		if(base_) {
			int result = base_->get_slot(key);
			if(result >= 0) {
				result += NUM_MAP_CALLABLE_SLOTS;
			}

			return result;
		} else {
			return -1;
		}
	}

	entry* get_entry(int slot) {
		if(slot < 0) {
			return NULL;
		}

		if(slot < entries_.size()) {
			return &entries_[slot];
		}

		if(base_) {
			return const_cast<formula_callable_definition*>(base_.get())->get_entry(slot - NUM_MAP_CALLABLE_SLOTS);
		}

		return NULL;
	}

	const entry* get_entry(int slot) const {
		if(slot < 0) {
			return NULL;
		}

		if(slot < entries_.size()) {
			return &entries_[slot];
		}

		if(base_) {
			return base_->get_entry(slot - NUM_MAP_CALLABLE_SLOTS);
		}

		return NULL;
	}

	int num_slots() const {
		return NUM_MAP_CALLABLE_SLOTS + (base_ ? base_->num_slots() : 0);
	}

private:
	const_formula_callable_definition_ptr base_;
	variant_type_ptr key_type_, value_type_;

	std::vector<entry> entries_;
};

class map_callable : public formula_callable {
	public:
		explicit map_callable(const formula_callable& backup)
		: backup_(&backup)
		{}

		void set_value_name(const std::string& name) { value_name_ = name; }

		void set(const variant& v, int i)
		{
			value_ = v;
			index_ = i;
		}

		void set(const variant& k, const variant& v, int i)
		{
			key_ = k;
			value_ = v;
			index_ = i;
		}
	private:
		variant get_value(const std::string& key) const {
			if(value_name_.empty() && key == "value" ||
			   !value_name_.empty() && key == value_name_) {
				return value_;
			} else if(key == "index") {
				return variant(index_);
			} else if(key == "context") {
				return variant(backup_.get());
			} else if(key == "key") {
				return key_;
			} else {
				return backup_->query_value(key);
			}
		}

		variant get_value_by_slot(int slot) const {
			ASSERT_LOG(slot >= 0, "BAD SLOT VALUE: " << slot);
			if(slot < NUM_MAP_CALLABLE_SLOTS) {
				switch(slot) {
					case MAP_CALLABLE_VALUE: return value_;
					case MAP_CALLABLE_INDEX: return variant(index_);
					case MAP_CALLABLE_CONTEXT: return variant(backup_.get());
					case MAP_CALLABLE_KEY: return key_;
					default: ASSERT_LOG(false, "BAD GET VALUE BY SLOT");
				}
			} else if(backup_) {
				return backup_->query_value_by_slot(slot - NUM_MAP_CALLABLE_SLOTS);
			} else {
				ASSERT_LOG(false, "COULD NOT FIND VALUE FOR SLOT: " << slot);
			}
		}

		const const_formula_callable_ptr backup_;
		variant key_;
		variant value_;
		int index_;

		std::string value_name_;
};

FUNCTION_DEF(count, 2, 2, "count(list, expr): Returns an integer count of how many items in the list 'expr' returns true for.")
	const variant items = split_variant_if_str(args()[0]->evaluate(variables));
	if(items.is_map()) {
		int res = 0;
		boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
		int index = 0;
		foreach(const variant_pair& p, items.as_map()) {
			callable->set(p.first, p.second, index);
			const variant val = args().back()->evaluate(*callable);
			if(val.as_bool()) {
				++res;
			}

			++index;
		}

		return variant(res);
	} else {
		int res = 0;
		boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
		for(size_t n = 0; n != items.num_elements(); ++n) {
			callable->set(items[n], n);
			const variant val = args().back()->evaluate(*callable);
			if(val.as_bool()) {
				++res;
			}
		}

		return variant(res);
	}

FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_INT);
END_FUNCTION_DEF(count)

class filter_function : public function_expression {
public:
	explicit filter_function(const args_list& args)
		: function_expression("filter", args, 2, 3)
	{
		if(args.size() == 3) {
			args[1]->is_identifier(&identifier_);
		}
	}
private:
	std::string identifier_;
	variant execute(const formula_callable& variables) const {
		std::vector<variant> vars;
		const variant items = args()[0]->evaluate(variables);
		if(args().size() == 2) {

			if(items.is_map()) {
				boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
				std::map<variant,variant> m;
				int index = 0;
				foreach(const variant_pair& p, items.as_map()) {
					callable->set(p.first, p.second, index);
					const variant val = args().back()->evaluate(*callable);
					if(val.as_bool()) {
						m[p.first] = p.second;
					}

					++index;
				}

				return variant(&m);
			} else {
				boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
				for(size_t n = 0; n != items.num_elements(); ++n) {
					callable->set(items[n], n);
					const variant val = args().back()->evaluate(*callable);
					if(val.as_bool()) {
						vars.push_back(items[n]);
					}
				}
			}
		} else {
			boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
			const std::string self = identifier_.empty() ? args()[1]->evaluate(variables).as_string() : identifier_;
			callable->set_value_name(self);

			for(size_t n = 0; n != items.num_elements(); ++n) {
				callable->set(items[n], n);
				const variant val = args().back()->evaluate(*callable);
				if(val.as_bool()) {
					vars.push_back(items[n]);
				}
			}
		}

		return variant(&vars);
	}

	variant_type_ptr get_variant_type() const {
		return args()[0]->query_variant_type();
	}
};
	
FUNCTION_DEF(mapping, -1, -1, "mapping(x): Turns the args passed in into a map. The first arg is a key, the second a value, the third a key, the fourth a value and so on and so forth.")
	map_formula_callable* callable = new map_formula_callable;
	for(size_t n = 0; n < args().size()-1; n += 2) {
		callable->add(args()[n]->evaluate(variables).as_string(),
					args()[n+1]->evaluate(variables));
	}
	
	return variant(callable);
END_FUNCTION_DEF(mapping)

class find_function : public function_expression {
public:
	explicit find_function(const args_list& args)
		: function_expression("find", args, 2, 3)
	{
		if(args.size() == 3) {
			args[1]->is_identifier(&identifier_);
		}
	}

private:
	std::string identifier_;
	variant execute(const formula_callable& variables) const {
		const variant items = args()[0]->evaluate(variables);

		if(args().size() == 2) {
			boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
			for(size_t n = 0; n != items.num_elements(); ++n) {
				callable->set(items[n], n);
				const variant val = args().back()->evaluate(*callable);
				if(val.as_bool()) {
					return items[n];
				}
			}
		} else {
			boost::intrusive_ptr<map_callable> callable(new map_callable(variables));

			const std::string self = identifier_.empty() ? args()[1]->evaluate(variables).as_string() : identifier_;
			callable->set_value_name(self);

			for(size_t n = 0; n != items.num_elements(); ++n) {
				callable->set(items[n], n);
				const variant val = args().back()->evaluate(*callable);
				if(val.as_bool()) {
					return items[n];
				}
			}
		}

		return variant();
	}

	variant_type_ptr get_variant_type() const {
		std::vector<variant_type_ptr> types;
		types.push_back(args()[0]->query_variant_type()->is_list_of());
		types.push_back(variant_type::get_type(variant::VARIANT_TYPE_NULL));
		return variant_type::get_union(types);
	}
};

	class transform_callable : public formula_callable {
	public:
		explicit transform_callable(const formula_callable& backup)
		: backup_(backup)
		{}

		void set(const variant& v, const variant& i)
		{
			value_ = v;
			index_ = i;
		}
	private:
		variant get_value(const std::string& key) const {
			if(key == "v") {
				return value_;
			} else if(key == "i") {
				return index_;
			} else {
				return backup_.query_value(key);
			}
		}

		variant get_value_by_slot(int slot) const {
			return backup_.query_value_by_slot(slot);
		}

		const formula_callable& backup_;
		variant value_, index_;
	};

FUNCTION_DEF(transform, 2, 2, "transform(list,ffl): calls the ffl for each item on the given list, returning a list of the results. Inside the transform v is the value of the list item and i is the index. e.g. transform([1,2,3], v+2) = [3,4,5] and transform([1,2,3], i) = [0,1,2]")
	std::vector<variant> vars;
	const variant items = args()[0]->evaluate(variables);

	vars.reserve(items.num_elements());

	transform_callable* callable = new transform_callable(variables);
	variant v(callable);

	const int nitems = items.num_elements();
	for(size_t n = 0; n != nitems; ++n) {
		callable->set(items[n], variant(unsigned(n)));
		const variant val = args().back()->evaluate(*callable);
		vars.push_back(val);
	}

	return variant(&vars);
END_FUNCTION_DEF(transform)

namespace {
void visit_objects(variant v, std::vector<variant>& res) {
	if(v.is_map()) {
		res.push_back(v);
		foreach(const variant_pair& value, v.as_map()) {
			visit_objects(value.second, res);
		}
	} else if(v.is_list()) {
		foreach(const variant& value, v.as_list()) {
			visit_objects(value, res);
		}
	} else if(v.try_convert<variant_callable>()) {
		res.push_back(v);
		variant keys = v.try_convert<variant_callable>()->get_value().get_keys();
		foreach(variant k, keys.as_list()) {
			visit_objects(v.try_convert<variant_callable>()->query_value(k.as_string()), res);
		}
	}
}
}

class visit_objects_function : public function_expression 
{
public:
	explicit visit_objects_function(const args_list& args)
		: function_expression("visit_objects", args, 1, 1)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const variant v = args()[0]->evaluate(variables);
		std::vector<variant> result;
		visit_objects(v, result);
		return variant(&result);
	}
};

FUNCTION_DEF(choose, 1, 2, "choose(list, (optional)scoring_expr) -> value: choose an item from the list according to which scores the highest according to the scoring expression, or at random by default.")

	if(args().size() == 1) {
		formula::fail_if_static_context();
	}

	const variant items = args()[0]->evaluate(variables);
	int max_index = -1;
	variant max_value;
	boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
	for(size_t n = 0; n != items.num_elements(); ++n) {
		variant val;
		
		if(args().size() >= 2) {
			callable->set(items[n], n);
			val = args().back()->evaluate(*callable);
		} else {
			val = variant(rand());
		}

		if(max_index == -1 || val > max_value) {
			max_index = n;
			max_value = val;
		}
	}

	if(max_index == -1) {
		return variant();
	} else {
		return items[max_index];
	}
FUNCTION_TYPE_DEF
	return args()[0]->query_variant_type()->is_list_of();
END_FUNCTION_DEF(choose)

class map_function : public function_expression {
public:
	explicit map_function(const args_list& args)
		: function_expression("map", args, 2, 3)
	{
		if(args.size() == 3) {
			args[1]->is_identifier(&identifier_);
		}
	}
private:
	std::string identifier_;

	variant execute(const formula_callable& variables) const {
		std::vector<variant> vars;
		const variant items = args()[0]->evaluate(variables);

		vars.reserve(items.num_elements());

		if(args().size() == 2) {

			if(items.is_map()) {
				boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
				int index = 0;
				foreach(const variant_pair& p, items.as_map()) {
					callable->set(p.first, p.second, index);
					const variant val = args().back()->evaluate(*callable);
					vars.push_back(val);
					++index;
				}
			} else if(items.is_string()) {
				const std::string& s = items.as_string();
				boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
				for(size_t n = 0; n != s.length(); ++n) {
					variant v(s.substr(n,1));
					callable->set(v, n);
					const variant val = args().back()->evaluate(*callable);
					vars.push_back(val);
				}
			} else {
				boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
				for(size_t n = 0; n != items.num_elements(); ++n) {
					callable->set(items[n], n);
					const variant val = args().back()->evaluate(*callable);
					vars.push_back(val);
				}
			}
		} else {
			boost::intrusive_ptr<map_callable> callable(new map_callable(variables));
			const std::string self = identifier_.empty() ? args()[1]->evaluate(variables).as_string() : identifier_;
			callable->set_value_name(self);
			for(size_t n = 0; n != items.num_elements(); ++n) {
				callable->set(items[n], n);
				const variant val = args().back()->evaluate(*callable);
				vars.push_back(val);
			}
		}

		return variant(&vars);
	}

	variant_type_ptr get_variant_type() const {
		return variant_type::get_list(args()[args().size()-1]->query_variant_type());
	}
};

FUNCTION_DEF(sum, 1, 2, "sum(list[, counter]): Adds all elements of the list together. If counter is supplied, all elements of the list are added to the counter instead of to 0.")
	variant res(0);
	const variant items = args()[0]->evaluate(variables);
	if(args().size() >= 2) {
		res = args()[1]->evaluate(variables);
	}
	for(size_t n = 0; n != items.num_elements(); ++n) {
		res = res + items[n];
	}

	return res;
FUNCTION_TYPE_DEF
	std::vector<variant_type_ptr> types;
	types.push_back(args()[0]->query_variant_type()->is_list_of());
	if(args().size() > 1) {
		types.push_back(args()[1]->query_variant_type());
	}

	return variant_type::get_union(types);

END_FUNCTION_DEF(sum)

FUNCTION_DEF(range, 1, 3, "range([start, ]finish[, step]): Returns a list containing all numbers smaller than the finish value and and larger than or equal to the start value. The start value defaults to 0.")
	int start = args().size() > 1 ? args()[0]->evaluate(variables).as_int() : 0;
	int end = args()[args().size() > 1 ? 1 : 0]->evaluate(variables).as_int();
	int step = args().size() < 3 ? 1 : args()[2]->evaluate(variables).as_int();
	ASSERT_LOG(step > 0, "ILLEGAL STEP VALUE IN RANGE: " << step);
	bool reverse = false;
	if(end < start) {
		std::swap(start, end);
		++start;
		++end;
		reverse = true;
	}
	const int nelem = end - start;

	std::vector<variant> v;

	if(nelem > 0) {
		v.reserve(nelem/step);

		for(int n = 0; n < nelem; n += step) {
			v.push_back(variant(start+n));
		}
	}

	if(reverse) {
		std::reverse(v.begin(), v.end());
	}

	return variant(&v);
FUNCTION_TYPE_DEF
	return variant_type::get_list(variant_type::get_type(variant::VARIANT_TYPE_INT));
END_FUNCTION_DEF(range)

FUNCTION_DEF(reverse, 1, 1, "reverse(list): reverses the given list")
	std::vector<variant> items = args()[0]->evaluate(variables).as_list();
	std::reverse(items.begin(), items.end());
	return variant(&items);
FUNCTION_TYPE_DEF
	return args()[0]->query_variant_type();
END_FUNCTION_DEF(reverse)

FUNCTION_DEF(head, 1, 1, "head(list): gives the first element of a list, or null for an empty list")
	const variant items = args()[0]->evaluate(variables);
	if(items.num_elements() >= 1) {
		return items[0];
	} else {
		return variant();
	}
FUNCTION_TYPE_DEF
	std::vector<variant_type_ptr> types;
	types.push_back(variant_type::get_type(variant::VARIANT_TYPE_NULL));
	types.push_back(args()[0]->query_variant_type()->is_list_of());
	return variant_type::get_union(types);
END_FUNCTION_DEF(head)

FUNCTION_DEF(back, 1, 1, "back(list): gives the last element of a list, or null for an empty list")
	const variant items = args()[0]->evaluate(variables);
	if(items.num_elements() >= 1) {
		return items[items.num_elements()-1];
	} else {
		return variant();
	}
FUNCTION_TYPE_DEF
	std::vector<variant_type_ptr> types;
	types.push_back(variant_type::get_type(variant::VARIANT_TYPE_NULL));
	types.push_back(args()[0]->query_variant_type()->is_list_of());
	return variant_type::get_union(types);
END_FUNCTION_DEF(back)

FUNCTION_DEF(get_all_files_under_dir, 1, 1, "get_all_files_under_dir(path): Returns a list of all the files in and under the given directory")
	std::vector<variant> v;
	std::map<std::string, std::string> file_paths;
	module::get_unique_filenames_under_dir(args()[0]->evaluate(variables).as_string(), &file_paths);
	for(std::map<std::string, std::string>::const_iterator i = file_paths.begin(); i != file_paths.end(); ++i) {
		//std::cerr << "FILE " << i->first << " : " << i->second << std::endl;
		v.push_back(variant(i->second));
	}
	return variant(&v);
FUNCTION_TYPE_DEF
	return variant_type::get_list(variant_type::get_type(variant::VARIANT_TYPE_STRING));
END_FUNCTION_DEF(get_all_files_under_dir)

FUNCTION_DEF(get_files_in_dir, 1, 1, "get_files_in_dir(path): Returns a list of the files in the given directory")
	std::vector<variant> v;
	std::vector<std::string> files;
	std::string dirname = args()[0]->evaluate(variables).as_string();
	if(dirname[dirname.size()-1] != '/') {
		dirname += '/';
	}
	module::get_files_in_dir(dirname, &files);
	for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i) {
		//std::cerr << "FILE " << *i << std::endl;
		v.push_back(variant(*i));
	}
	return variant(&v);
FUNCTION_TYPE_DEF
	return variant_type::get_list(variant_type::get_type(variant::VARIANT_TYPE_STRING));
END_FUNCTION_DEF(get_files_in_dir)

FUNCTION_DEF(dialog, 2, 2, "dialog(obj, template): Creates a dialog given an object to operate on and a template for the dialog.")
	bool modal = args().size() == 3 && args()[2]->evaluate(variables).as_bool(); 
	variant environment = args()[0]->evaluate(variables);
	variant dlg_template = args()[1]->evaluate(variables);
	formula_callable* e = environment.try_convert<formula_callable>();
	variant v;
	if(dlg_template.is_string()) {
		std::string s = dlg_template.as_string();
		if(s.length() <= 4 || s.substr(s.length()-4) != ".cfg") {
			s += ".cfg";
		}
		v = json::parse_from_file(gui::get_dialog_file(s));
	} else {
		v = dlg_template;
	}
	return variant(new gui::dialog(v, e));
END_FUNCTION_DEF(dialog)

FUNCTION_DEF(show_modal, 1, 1, "show_modal(dialog): Displays a modal dialog on the screen.")
	variant graph = args()[0]->evaluate(variables);
	gui::dialog_ptr dialog = boost::intrusive_ptr<gui::dialog>(graph.try_convert<gui::dialog>());
	ASSERT_LOG(dialog, "Dialog given is not of the correct type.");
	dialog->show_modal();
	return variant::from_bool(dialog->cancelled() == false);
END_FUNCTION_DEF(show_modal)

FUNCTION_DEF(index, 2, 2, "index(list, value) -> index of value in list: Returns the index of the value in the list or -1 if value wasn't found in the list.")
	variant value = args()[1]->evaluate(variables);
	variant li = args()[0]->evaluate(variables);
	for(int n = 0; n < li.num_elements(); n++) {
		if(value == li[n]) {
			return variant(n);
		}
	}
	return variant(-1);
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_INT);
END_FUNCTION_DEF(index)

namespace {
void evaluate_expr_for_benchmark(const formula_expression* expr, const formula_callable* variables, int ntimes)
{
	for(int n = 0; n < ntimes; ++n) {
		expr->evaluate(*variables);
	}
}

}

FUNCTION_DEF(benchmark, 1, 1, "benchmark(expr): Executes expr in a benchmark harness and returns a string describing its benchmark performance")
	return variant(test::run_benchmark("benchmark", boost::bind(evaluate_expr_for_benchmark, args()[0].get(), &variables, _1)));
END_FUNCTION_DEF(benchmark)

FUNCTION_DEF(compress, 1, 2, "compress(string, (optional) compression_level): Compress the given string object")
	int compression_level = -1;
	if(args().size() > 1) {
		compression_level = args()[1]->evaluate(variables).as_int();
	}
	const std::string s = args()[0]->evaluate(variables).as_string();
	return variant(new zip::compressed_data(std::vector<char>(s.begin(), s.end()), compression_level));
END_FUNCTION_DEF(compress)

FUNCTION_DEF(decompress, 1, 1, "decompress(expr): Tries to decompress the given object, returns the data if successful.")
	variant compressed = args()[0]->evaluate(variables);
	zip::compressed_data_ptr cd = boost::intrusive_ptr<zip::compressed_data>(compressed.try_convert<zip::compressed_data>());
	if(cd == NULL) {
		ASSERT_LOG(compressed.is_string(), "decompress takes either a compressed_data object or string.");
		std::string s = compressed.as_string();
		std::string key = s.length() > 10 ? s.substr(0,10) : s;
		int n;
		if((n = s.find(':')) != std::string::npos) {
			key = s.substr(0, n);
			s = s.substr(n+1);
		}
		std::vector<char> vc = zip::decompress(base64::b64decode(std::vector<char>(s.begin(), s.end())));
		return variant(new data_blob(key, vc));
	} else {
		return cd->get_value("decompress");
	}
END_FUNCTION_DEF(decompress)

FUNCTION_DEF(unencode, 1, 1, "unencode(expr) -> data_blob: Tries to unencode the given base64 encoded data.")
	variant encoded = args()[0]->evaluate(variables);
	ASSERT_LOG(encoded.is_string(), "encoded data must be a string");
	std::string s = encoded.as_string();
	std::string key = s.length() > 10 ? s.substr(0,10) : s;
	int n;
	if((n = s.find(':')) != std::string::npos) {
		key = s.substr(0, n);
		s = s.substr(n+1);
	}
	std::vector<char> vc = base64::b64decode(std::vector<char>(s.begin(), s.end()));
	return variant(new data_blob(key, vc));
END_FUNCTION_DEF(unencode)

	class size_function : public function_expression {
	public:
		explicit size_function(const args_list& args)
			: function_expression("size", args, 1, 1)
		{}
	private:
		variant execute(const formula_callable& variables) const {
			const variant items = args()[0]->evaluate(variables);
			if(items.is_string()) {
				return variant(items.as_string().size());
			}
			return variant(static_cast<int>(items.num_elements()));
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_INT);
		}
	};

	
	class split_function : public function_expression {
	public:
		explicit split_function(const args_list& args)
		: function_expression("split", args, 1, 2)
		{}
	private:
		variant execute(const formula_callable& variables) const {
			std::vector<std::string> chopped;
			if(args().size() >= 2) {
				const std::string thestring = args()[0]->evaluate(variables).as_string();
				const std::string delimiter = args()[1]->evaluate(variables).as_string();
				chopped = util::split(thestring, delimiter);
			} else {
				const std::string thestring = args()[0]->evaluate(variables).as_string();
				chopped = util::split(thestring);
			}
		
			std::vector<variant> res;
			for(size_t i=0; i<chopped.size(); ++i) {
				const std::string& part = chopped[i];
				res.push_back(variant(part));
			}
			
			return variant(&res);
			
		}

		variant_type_ptr get_variant_type() const {
			return variant_type::get_list(args()[0]->query_variant_type());
		}
	};
	
	class slice_function : public function_expression {
	public:
		explicit slice_function(const args_list& args)
			: function_expression("slice", args, 3, 3)
		{}
	private:
		variant execute(const formula_callable& variables) const {
			const variant list = args()[0]->evaluate(variables);
			if(list.num_elements() == 0) {
				return variant();
			}
			int begin_index = args()[1]->evaluate(variables).as_int()%(list.num_elements()+1);
			int end_index = args()[2]->evaluate(variables).as_int()%(list.num_elements()+1);
			if(end_index >= begin_index) {
				std::vector<variant> result;
				result.reserve(end_index - begin_index);
				while(begin_index != end_index) {
					result.push_back(list[begin_index++]);
				}

				return variant(&result);
			} else {
				return variant();
			}
		}
	};

FUNCTION_DEF(int, 1, -1, "overload(fn...): makes an overload of functions")
	return variant(args()[0]->evaluate(variables).as_int());
FUNCTION_TYPE_DEF
	return variant_type::get_type(variant::VARIANT_TYPE_INT);
END_FUNCTION_DEF(int)

	class str_function : public function_expression {
	public:
		explicit str_function(const args_list& args)
			: function_expression("str", args, 1, 1)
		{}
	private:
		variant execute(const formula_callable& variables) const {
			const variant item = args()[0]->evaluate(variables);
			if(item.is_string()) {
				//just return as-is for something that's already a string.
				return item;
			}

			std::string str;
			item.serialize_to_string(str);
			return variant(str);
		}

		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_STRING);
		}
	};

	class strstr_function : public function_expression {
	public:
		explicit strstr_function(const args_list& args)
		: function_expression("strstr", args, 2, 2)
		{}
	private:
		variant execute(const formula_callable& variables) const {
			const std::string haystack = args()[0]->evaluate(variables).as_string();
			const std::string needle = args()[1]->evaluate(variables).as_string();

			const size_t pos = haystack.find(needle);

			if(pos == std::string::npos) {
				return variant(0);
			} else {
				return variant(pos + 1);
			}
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_INT);
		}
	};

	class null_function : public function_expression {
	public:
		explicit null_function(const args_list& args)
			: function_expression("null", args, 0, 0)
		{}
	private:
		variant execute(const formula_callable& /*variables*/) const {
			return variant();
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_NULL);
		}
	};

	class refcount_function : public function_expression {
	public:
		explicit refcount_function(const args_list& args)
			: function_expression("refcount", args, 1, 1)
		{}
	private:
		variant execute(const formula_callable& variables) const {
			return variant(args()[0]->evaluate(variables).refcount());
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_INT);
		}
	};

	class deserialize_function : public function_expression {
	public:
		explicit deserialize_function(const args_list& args)
		: function_expression("deserialize", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			formula::fail_if_static_context();
			const intptr_t id = strtoll(args()[0]->evaluate(variables).as_string().c_str(), NULL, 16);
			return variant::create_variant_under_construction(id);
		}
	};

	class is_string_function : public function_expression {
	public:
		explicit is_string_function(const args_list& args)
			: function_expression("is_string", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			return variant(args()[0]->evaluate(variables).is_string());
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		}
	};

	class is_null_function : public function_expression {
	public:
		explicit is_null_function(const args_list& args)
			: function_expression("is_null", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			return variant(args()[0]->evaluate(variables).is_null());
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		}
	};

	class is_int_function : public function_expression {
	public:
		explicit is_int_function(const args_list& args)
			: function_expression("is_int", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			return variant(args()[0]->evaluate(variables).is_int());
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		}
	};

	class is_bool_function : public function_expression {
	public:
		explicit is_bool_function(const args_list& args)
			: function_expression("is_bool", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			return variant(args()[0]->evaluate(variables).is_bool());
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		}
	};

	class is_decimal_function : public function_expression {
	public:
		explicit is_decimal_function(const args_list& args)
			: function_expression("is_decimal", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			return variant(args()[0]->evaluate(variables).is_decimal());
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		}
	};

	class is_map_function : public function_expression {
	public:
		explicit is_map_function(const args_list& args)
			: function_expression("is_map", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			return variant(args()[0]->evaluate(variables).is_map());
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		}
	};

	class mod_function : public function_expression {
		//the standard C++ mod expression does not give correct answers for negative operands - it's "implementation-defined", which means it's not really a modulo operation the way math normally describes them.  To get the right answer, we're using the following - based on the fact that x%y is always in the range [-y+1, y-1], and thus adding y to it is both always enough to make it positive, but doesn't change the modulo value.
	public:
		explicit mod_function(const args_list& args)
		: function_expression("mod", args, 2, 2)
		{}
		
	private:
		variant execute(const formula_callable& variables) const {
			int left = args()[0]->evaluate(variables).as_int();
			int right = args()[1]->evaluate(variables).as_int();
			
			return variant((left%right + right)%right);
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_INT);
		}
	};
	
	class is_function_function : public function_expression {
	public:
		explicit is_function_function(const args_list& args)
			: function_expression("is_function", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			return variant(args()[0]->evaluate(variables).is_function());
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		}
	};

	class is_list_function : public function_expression {
	public:
		explicit is_list_function(const args_list& args)
			: function_expression("is_list", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			return variant(args()[0]->evaluate(variables).is_list());
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		}
	};

	class is_callable_function : public function_expression {
	public:
		explicit is_callable_function(const args_list& args)
			: function_expression("is_callable", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			return variant(args()[0]->evaluate(variables).is_callable());
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_type(variant::VARIANT_TYPE_BOOL);
		}
	};

	class list_str_function : public function_expression {
	public:
		explicit list_str_function(const args_list& args)
			: function_expression("list_str", args, 1, 1)
		{}

	private:
		variant execute(const formula_callable& variables) const {
			const std::string str = args()[0]->evaluate(variables).as_string();
			std::vector<variant> result;
			
			int count = 0;
			while (str[count] != 0) {
				std::string chr(1,str[count]);
				result.push_back(variant(chr));
				count++;
			}
			return variant(&result);
		}
		variant_type_ptr get_variant_type() const {
			return variant_type::get_list(variant_type::get_type(variant::VARIANT_TYPE_STRING));
		}
	};

class set_command : public game_logic::command_callable
{
public:
	set_command(variant target, const std::string& attr, const variant& variant_attr, variant val)
	  : target_(target), attr_(attr), variant_attr_(variant_attr), val_(val)
	{}
	virtual void execute(game_logic::formula_callable& ob) const {
		if(target_.is_callable()) {
			ASSERT_LOG(!attr_.empty(), "ILLEGAL KEY IN SET OF CALLABLE: " << val_.write_json());
			target_.mutable_callable()->mutate_value(attr_, val_);
		} else if(target_.is_map()) {
			if(!attr_.empty()) {
				target_.add_attr_mutation(variant(attr_), val_);
			} else {
				target_.add_attr_mutation(variant_attr_, val_);
			}
		} else {
			ASSERT_LOG(!attr_.empty(), "ILLEGAL KEY IN SET OF CALLABLE: " << val_.write_json());
			ob.mutate_value(attr_, val_);
		}
	}
private:
	mutable variant target_;
	std::string attr_;
	variant variant_attr_;
	variant val_;
};

class add_command : public game_logic::command_callable
{
public:
	add_command(variant target, const std::string& attr, const variant& variant_attr, variant val)
	  : target_(target), attr_(attr), variant_attr_(variant_attr), val_(val)
	{}
	virtual void execute(game_logic::formula_callable& ob) const {
		if(target_.is_callable()) {
			ASSERT_LOG(!attr_.empty(), "ILLEGAL KEY IN ADD OF CALLABLE: " << val_.write_json());
			target_.mutable_callable()->mutate_value(attr_, target_.mutable_callable()->query_value(attr_) + val_);
		} else if(target_.is_map()) {
			if(!attr_.empty()) {
				variant key(attr_);
				target_.add_attr_mutation(key, target_[key] + val_);
			} else {
				target_.add_attr_mutation(variant_attr_, target_[variant_attr_] + val_);
			}
		} else {
			ASSERT_LOG(!attr_.empty(), "ILLEGAL KEY IN ADD OF CALLABLE: " << val_.write_json());
			ob.mutate_value(attr_, ob.query_value(attr_) + val_);
		}
	}
private:
	mutable variant target_;
	std::string attr_;
	variant variant_attr_;
	variant val_;
};

class set_by_slot_command : public game_logic::command_callable
{
public:
	set_by_slot_command(int slot, const variant& value)
	  : slot_(slot), value_(value)
	{}

	virtual void execute(game_logic::formula_callable& obj) const {
		obj.mutate_value_by_slot(slot_, value_);
	}

	void set_value(const variant& value) { value_ = value; }

private:
	int slot_;
	variant value_;
};

class add_by_slot_command : public game_logic::command_callable
{
public:
	add_by_slot_command(int slot, const variant& value)
	  : slot_(slot), value_(value)
	{}

	virtual void execute(game_logic::formula_callable& obj) const {
		obj.mutate_value_by_slot(slot_, obj.query_value_by_slot(slot_) + value_);
	}

	void set_value(const variant& value) { value_ = value; }

private:
	int slot_;
	variant value_;
};

class set_function : public function_expression {
public:
	set_function(const args_list& args, const_formula_callable_definition_ptr callable_def)
	  : function_expression("set", args, 2, 3), slot_(-1) {
		if(args.size() == 2) {
			variant literal;
			args[0]->is_literal(literal);
			if(literal.is_string()) {
				key_ = literal.as_string();
			} else {
				args[0]->is_identifier(&key_);
			}

			if(!key_.empty() && callable_def) {
				slot_ = callable_def->get_slot(key_);
				if(slot_ != -1) {
					cmd_ = boost::intrusive_ptr<set_by_slot_command>(new set_by_slot_command(slot_, variant()));
				}
			}
		}
	}
private:
	variant execute(const formula_callable& variables) const {
		if(slot_ != -1) {
			if(cmd_->refcount() == 1) {
				cmd_->set_value(args()[1]->evaluate(variables));
				cmd_->set_expression(this);
				return variant(cmd_.get());
			}

			cmd_ = boost::intrusive_ptr<set_by_slot_command>(new set_by_slot_command(slot_, args()[1]->evaluate(variables)));
			cmd_->set_expression(this);
			return variant(cmd_.get());
		}

		if(!key_.empty()) {
			static const std::string MeKey = "me";
			variant target = variables.query_value(MeKey);
			set_command* cmd = new set_command(target, key_, variant(), args()[1]->evaluate(variables));
			cmd->set_expression(this);
			return variant(cmd);
		}

		if(args().size() == 2) {
			std::string member;
			variant variant_member;
			variant target = args()[0]->evaluate_with_member(variables, member, &variant_member);
			set_command* cmd = new set_command(
			  target, member, variant_member, args()[1]->evaluate(variables));
			cmd->set_expression(this);
			return variant(cmd);
		}

		variant target;
		if(args().size() == 3) {
			target = args()[0]->evaluate(variables);
		}
		const int begin_index = args().size() == 2 ? 0 : 1;
		set_command* cmd = new set_command(
		    target,
		    args()[begin_index]->evaluate(variables).as_string(), variant(),
			args()[begin_index + 1]->evaluate(variables));
		cmd->set_expression(this);
		return variant(cmd);
	}

	variant_type_ptr get_variant_type() const {
		return variant_type::get_list(variant_type::get_type(variant::VARIANT_TYPE_CALLABLE));
	}

	std::string key_;
	int slot_;
	mutable boost::intrusive_ptr<set_by_slot_command> cmd_;
};

class add_function : public function_expression {
public:
	add_function(const args_list& args, const_formula_callable_definition_ptr callable_def)
	  : function_expression("add", args, 2, 3), slot_(-1) {
		if(args.size() == 2) {
			variant literal;
			args[0]->is_literal(literal);
			if(literal.is_string()) {
				key_ = literal.as_string();
			} else {
				args[0]->is_identifier(&key_);
			}

			if(!key_.empty() && callable_def) {
				slot_ = callable_def->get_slot(key_);
				if(slot_ != -1) {
					cmd_ = boost::intrusive_ptr<add_by_slot_command>(new add_by_slot_command(slot_, variant()));
				}
			}
		}
	}
private:
	variant execute(const formula_callable& variables) const {
		if(slot_ != -1) {
			if(cmd_->refcount() == 1) {
				cmd_->set_value(args()[1]->evaluate(variables));
				cmd_->set_expression(this);
				return variant(cmd_.get());
			}

			cmd_ = boost::intrusive_ptr<add_by_slot_command>(new add_by_slot_command(slot_, args()[1]->evaluate(variables)));
			cmd_->set_expression(this);
			return variant(cmd_.get());
		}

		if(!key_.empty()) {
			static const std::string MeKey = "me";
			variant target = variables.query_value(MeKey);
			add_command* cmd = new add_command(target, key_, variant(), args()[1]->evaluate(variables));
			cmd->set_expression(this);
			return variant(cmd);
		}

		if(args().size() == 2) {
			std::string member;
			variant variant_member;
			variant target = args()[0]->evaluate_with_member(variables, member, &variant_member);
			add_command* cmd = new add_command(
			      target, member, variant_member, args()[1]->evaluate(variables));
			cmd->set_expression(this);
			return variant(cmd);
		}

		variant target;
		if(args().size() == 3) {
			target = args()[0]->evaluate(variables);
		}
		const int begin_index = args().size() == 2 ? 0 : 1;
		add_command* cmd = new add_command(
		    target,
		    args()[begin_index]->evaluate(variables).as_string(), variant(),
			args()[begin_index + 1]->evaluate(variables));
		cmd->set_expression(this);
		return variant(cmd);
	}

	variant_type_ptr get_variant_type() const {
		return variant_type::get_list(variant_type::get_type(variant::VARIANT_TYPE_CALLABLE));
	}

	std::string key_;
	int slot_;
	mutable boost::intrusive_ptr<add_by_slot_command> cmd_;
};


class debug_command : public game_logic::command_callable
{
public:
	explicit debug_command(const std::string& str) : str_(str)
	{}
	virtual void execute(formula_callable& ob) const {
#ifndef NO_EDITOR
		debug_console::add_message(str_);
#endif
		std::cerr << "CONSOLE: " << str_ << "\n";
	}
private:
	std::string str_;
};

FUNCTION_DEF(debug, 1, -1, "debug(...): outputs arguments to the console")
	if(!preferences::debug()) {
		return variant();
	}

	std::string str;
	for(int n = 0; n != args().size(); ++n) {
		if(n > 0) {
			str += " ";
		}

		str += args()[n]->evaluate(variables).to_debug_string();
	}

	//fprintf(stderr, "DEBUG FUNCTION: %s\n", str.c_str());

	return variant(new debug_command(str));
END_FUNCTION_DEF(debug)

namespace {
void debug_side_effect(variant v)
{
	if(v.is_list()) {
		foreach(variant item, v.as_list()) {
			debug_side_effect(item);
		}
	} else if(v.is_callable() && v.try_convert<game_logic::command_callable>()) {
		map_formula_callable_ptr obj(new map_formula_callable);
		v.try_convert<game_logic::command_callable>()->run_command(*obj);
	} else {
		std::string s = v.to_debug_string();
#ifndef NO_EDITOR
		debug_console::add_message(s);
#endif
		std::cerr << "CONSOLE: " << s << "\n";
	}
}
}

FUNCTION_DEF(debug_fn, 2, 2, "debug_fn(msg, expr): evaluates and returns expr. Will print 'msg' to stderr if it's printable, or execute it if it's an executable command.")
	variant res = args()[1]->evaluate(variables);
	if(preferences::debug()) {
		debug_side_effect(args()[0]->evaluate(variables));
	}

	return res;
END_FUNCTION_DEF(debug_fn)

namespace {
bool consecutive_periods(char a, char b) {
	return a == '.' && b == '.';
}

std::map<std::string, variant>& get_doc_cache() {
	static std::map<std::string, variant> cache;
	return cache;
}
}

FUNCTION_DEF(write_document, 2, 2, "write_document(string filename, doc): writes 'doc' to the given filename")
	formula::fail_if_static_context();
	std::string docname = args()[0]->evaluate(variables).as_string();
	variant doc = args()[1]->evaluate(variables);

	if(docname.empty()) {
		return variant("DOCUMENT NAME GIVEN TO write_document() IS EMPTY");
	}
	if(sys::is_path_absolute(docname)) {
		return variant(formatter() << "DOCUMENT NAME IS ABSOLUTE PATH " << docname);
	}
	if(std::adjacent_find(docname.begin(), docname.end(), consecutive_periods) != docname.end()) {
		return variant(formatter() << "RELATIVE PATH OUTSIDE ALLOWED " << docname);
	}

	get_doc_cache()[docname] = doc;

	docname = preferences::user_data_path() + docname;
	sys::write_file(docname, doc.write_json());
	return variant();
END_FUNCTION_DEF(write_document)

FUNCTION_DEF(get_document, 1, 2, "get_document(string filename, [string] flags): return reference to the given JSON document. flags can contain 'null_on_failure' and 'user_preferences_dir'")
	formula::fail_if_static_context();
	std::string docname = args()[0]->evaluate(variables).as_string();
	ASSERT_LOG(docname.empty() == false, "DOCUMENT NAME GIVEN TO get_document() IS EMPTY");

	bool allow_failure = false;
	bool prefs_directory = false;

	if(args().size() > 1) {
		const variant flags = args()[1]->evaluate(variables);
		for(int n = 0; n != flags.num_elements(); ++n) {
			const std::string& flag = flags[n].as_string();
			if(flag == "null_on_failure") {
				allow_failure = true;
			} else if(flag == "user_preferences_dir") {
				prefs_directory = true;
			} else {
				ASSERT_LOG(false, "illegal flag given to get_document: " << flag);
			}
		}
	}

	variant& v = get_doc_cache()[docname];
	if(v.is_null() == false) {
		return v;
	}

	ASSERT_LOG(std::adjacent_find(docname.begin(), docname.end(), consecutive_periods) == docname.end(), "DOCUMENT NAME CONTAINS ADJACENT PERIODS " << docname);

	if(prefs_directory) {
		//docname = sys::compute_relative_path(preferences::user_data_path(), docname);
		docname = preferences::user_data_path() + docname;
	} else {
		ASSERT_LOG(!sys::is_path_absolute(docname), "DOCUMENT NAME USES AN ABSOLUTE PATH WHICH IS NOT ALLOWED: " << docname);
	}

	try {
		const variant v = json::parse_from_file(docname);
		return v;
	} catch(json::parse_error& e) {
		if(allow_failure) {
			return variant();
		}

		ASSERT_LOG(false, "COULD NOT LOAD DOCUMENT: " << e.error_message());
		return variant();
	}
FUNCTION_TYPE_DEF
	std::vector<variant_type_ptr> types;
	types.push_back(variant_type::get_type(variant::VARIANT_TYPE_MAP));
	types.push_back(variant_type::get_type(variant::VARIANT_TYPE_NULL));
	return variant_type::get_union(types);
END_FUNCTION_DEF(get_document)

}

void remove_formula_function_cached_doc(const std::string& name)
{
	get_doc_cache().erase(name);
}

formula_function_expression::formula_function_expression(const std::string& name, const args_list& args, const_formula_ptr formula, const_formula_ptr precondition, const std::vector<std::string>& arg_names, const std::vector<variant_type_ptr>& variant_types)
: function_expression(name, args, arg_names.size(), arg_names.size()),
	formula_(formula), precondition_(precondition), arg_names_(arg_names), variant_types_(variant_types), star_arg_(-1), has_closure_(false), base_slot_(0)
{
	assert(!precondition_ || !precondition_->str().empty());
	for(size_t n = 0; n != arg_names_.size(); ++n) {
		if(arg_names_.empty() == false && arg_names_[n][arg_names_[n].size()-1] == '*') {
			arg_names_[n].resize(arg_names_[n].size()-1);
			star_arg_ = n;
			break;
		}
	}
}

namespace {
std::stack<const formula_function_expression*> formula_fn_stack;
struct formula_function_scope {
	explicit formula_function_scope(const formula_function_expression* f) {
		formula_fn_stack.push(f);
	}

	~formula_function_scope() {
		formula_fn_stack.pop();
	}
};

bool is_calculating_recursion = false;
struct recursion_calculation_scope {
	recursion_calculation_scope() { is_calculating_recursion = true; }
	~recursion_calculation_scope() { is_calculating_recursion = false; }
};


}

boost::intrusive_ptr<slot_formula_callable> formula_function_expression::calculate_args_callable(const formula_callable& variables) const
{
	if(!callable_ || callable_->refcount() != 1) {
		callable_ = boost::intrusive_ptr<slot_formula_callable>(new slot_formula_callable);
		callable_->reserve(arg_names_.size());
		callable_->set_base_slot(base_slot_);
	}

	callable_->set_names(&arg_names_);

	//we reset callable_ to NULL during any calls so that recursive calls
	//will work properly.
	boost::intrusive_ptr<slot_formula_callable> tmp_callable(callable_);
	callable_.reset(NULL);

	for(int n = 0; n != arg_names_.size(); ++n) {
		variant var = args()[n]->evaluate(variables);

		if(n < variant_types_.size() && variant_types_[n]) {
			ASSERT_LOG(variant_types_[n]->match(var), "FUNCTION ARGUMENT " << (n+1) << " EXPECTED TYPE " << variant_types_[n]->str() << " BUT FOUND " << var.write_json() << " AT " << debug_pinpoint_location());
		}

		tmp_callable->add(var);
		if(n == star_arg_) {
			tmp_callable->set_fallback(var.as_callable());
		}
	}

	return tmp_callable;
}

variant formula_function_expression::execute(const formula_callable& variables) const
{
	if(fed_result_) {
		variant result = *fed_result_;
		fed_result_.reset();
		return result;
	}

	boost::intrusive_ptr<slot_formula_callable> tmp_callable = calculate_args_callable(variables);

	if(precondition_) {
		if(!precondition_->execute(*tmp_callable).as_bool()) {
			std::cerr << "FAILED function precondition (" << precondition_->str() << ") for function '" << formula_->str() << "' with arguments: ";
			for(size_t n = 0; n != arg_names_.size(); ++n) {
				std::cerr << "  arg " << (n+1) << ": " << args()[n]->evaluate(variables).to_debug_string() << "\n";
			}
		}
	}

	if(!is_calculating_recursion && formula_->has_guards() && !formula_fn_stack.empty() && formula_fn_stack.top() == this) {
		const recursion_calculation_scope recursion_scope;

		typedef boost::intrusive_ptr<formula_callable> call_ptr;
		std::vector<call_ptr> invocations;
		invocations.push_back(tmp_callable);
		while(formula_->guard_matches(*invocations.back()) == -1) {
			invocations.push_back(calculate_args_callable(*formula_->wrap_callable_with_global_where(*invocations.back())));
		}

		invocations.pop_back();

		if(invocations.size() > 2) {
			while(invocations.empty() == false) {
				fed_result_.reset(new variant(formula_->expr()->evaluate(*formula_->wrap_callable_with_global_where(*invocations.back()))));
				invocations.pop_back();
			}

			variant result = *fed_result_;
			fed_result_.reset();
			return result;
		}
	}

	formula_function_scope scope(this);
	variant res = formula_->execute(*tmp_callable);

	callable_ = tmp_callable;
	callable_->clear();

	return res;
}

	formula_function_expression_ptr formula_function::generate_function_expression(const std::vector<expression_ptr>& args_input) const
	{
		std::vector<expression_ptr> args = args_input;
		if(args.size() + default_args_.size() >= args_.size()) {
			const int base = args_.size() - default_args_.size();
			while(args.size() < args_.size()) {
				const int index = args.size() - base;
				ASSERT_LOG(index >= 0 && index < default_args_.size(), "INVALID INDEX INTO DEFAULT ARGS: " << index << " / " << default_args_.size());
				args.push_back(expression_ptr(new variant_expression(default_args_[index])));
			}
		}

		return formula_function_expression_ptr(new formula_function_expression(name_, args, formula_, precondition_, args_, variant_types_));
	}

	void function_symbol_table::add_formula_function(const std::string& name, const_formula_ptr formula, const_formula_ptr precondition, const std::vector<std::string>& args, const std::vector<variant>& default_args, const std::vector<variant_type_ptr>& variant_types)
	{
		custom_formulas_[name] = formula_function(name, formula, precondition, args, default_args, variant_types);
	}

	expression_ptr function_symbol_table::create_function(const std::string& fn, const std::vector<expression_ptr>& args, const_formula_callable_definition_ptr callable_def) const
	{

		const std::map<std::string, formula_function>::const_iterator i = custom_formulas_.find(fn);
		if(i != custom_formulas_.end()) {
			return i->second.generate_function_expression(args);
		}

		if(backup_) {
			return backup_->create_function(fn, args, callable_def);
		}

		return expression_ptr();
	}

	std::vector<std::string> function_symbol_table::get_function_names() const
	{
		std::vector<std::string> res;
		for(std::map<std::string, formula_function>::const_iterator iter = custom_formulas_.begin(); iter != custom_formulas_.end(); iter++ ) {
			res.push_back((*iter).first);
		}
		return res;
	}

	const formula_function* function_symbol_table::get_formula_function(const std::string& fn) const
	{
		const std::map<std::string, formula_function>::const_iterator i = custom_formulas_.find(fn);
		if(i == custom_formulas_.end()) {
			return NULL;
		} else {
			return &i->second;
		}
	}

	recursive_function_symbol_table::recursive_function_symbol_table(const std::string& fn, const std::vector<std::string>& args, const std::vector<variant>& default_args, function_symbol_table* backup, const_formula_callable_definition_ptr closure_definition, const std::vector<variant_type_ptr>& variant_types)
	: name_(fn), stub_(fn, const_formula_ptr(), const_formula_ptr(), args, default_args, variant_types), backup_(backup), closure_definition_(closure_definition)
	{
	}

	expression_ptr recursive_function_symbol_table::create_function(
					const std::string& fn,
					const std::vector<expression_ptr>& args,
					const_formula_callable_definition_ptr callable_def) const
	{
		if(fn == name_) {
			formula_function_expression_ptr expr = stub_.generate_function_expression(args);
			if(closure_definition_) {
				expr->set_has_closure(closure_definition_->num_slots());
			}
			expr_.push_back(expr);
			return expr;
		} else if(backup_) {
			return backup_->create_function(fn, args, callable_def);
		}

		return expression_ptr();
	}

	void recursive_function_symbol_table::resolve_recursive_calls(const_formula_ptr f)
	{
		foreach(formula_function_expression_ptr& fn, expr_) {
			fn->set_formula(f);
		}
	}

namespace {

	typedef std::map<std::string, function_creator*> functions_map;

	functions_map& get_functions_map() {

		static functions_map functions_table;

		if(functions_table.empty()) {
	#define FUNCTION(name) functions_table[#name] = new specific_function_creator<name##_function>();
			FUNCTION(if);
			FUNCTION(filter);
			FUNCTION(mapping);
			FUNCTION(find);
			FUNCTION(visit_objects);
			FUNCTION(map);
			FUNCTION(sum);
			FUNCTION(range);
			FUNCTION(head);
			FUNCTION(size);
			FUNCTION(split);
			FUNCTION(slice);
			FUNCTION(str);
			FUNCTION(strstr);
			FUNCTION(null);
			FUNCTION(refcount);
			FUNCTION(deserialize);
			FUNCTION(is_string);
			FUNCTION(is_null);
			FUNCTION(is_int);
			FUNCTION(is_decimal);
			FUNCTION(is_map);
			FUNCTION(mod);
			FUNCTION(is_function);
			FUNCTION(is_list);
			FUNCTION(is_callable);
			FUNCTION(list_str);
	#undef FUNCTION
		}

		return functions_table;
	}

}

expression_ptr create_function(const std::string& fn,
                               const std::vector<expression_ptr>& args,
							   const function_symbol_table* symbols,
							   const_formula_callable_definition_ptr callable_def)
{
	if(fn == "set") {
		return expression_ptr(new set_function(args, callable_def));
	} else if(fn == "add") {
		return expression_ptr(new add_function(args, callable_def));
	}

	if(symbols) {
		expression_ptr res(symbols->create_function(fn, args, callable_def));
		if(res) {
			return res;
		}
	}

	const std::map<std::string, function_creator*>& creators = get_function_creators(FunctionModule);
	std::map<std::string, function_creator*>::const_iterator creator_itor = creators.find(fn);
	if(creator_itor != creators.end()) {
		return expression_ptr(creator_itor->second->create(args));
	}

	functions_map::const_iterator i = get_functions_map().find(fn);
	if(i == get_functions_map().end()) {
		return expression_ptr();
	}

	return expression_ptr(i->second->create(args));
}

std::vector<std::string> builtin_function_names()
{
	std::vector<std::string> res;
	const functions_map& m = get_functions_map();
	for(functions_map::const_iterator i = m.begin(); i != m.end(); ++i) {
		res.push_back(i->first);
	}

	return res;
}

function_expression::function_expression(
                    const std::string& name,
                    const args_list& args,
                    int min_args, int max_args)
    : name_(name), args_(args), min_args_(min_args), max_args_(max_args)
{
	set_name(name.c_str());
}

void function_expression::set_debug_info(const variant& parent_formula,
	                            std::string::const_iterator begin_str,
	                            std::string::const_iterator end_str)
{
	formula_expression::set_debug_info(parent_formula, begin_str, end_str);

	if(min_args_ >= 0 && args_.size() < static_cast<size_t>(min_args_) ||
	   max_args_ >= 0 && args_.size() > static_cast<size_t>(max_args_)) {
		ASSERT_LOG(false, "ERROR: incorrect number of arguments to function '" << name_ << "': expected between " << min_args_ << " and " << max_args_ << ", found " << args_.size() << "\n" << debug_pinpoint_location());
	}
}

namespace {
bool point_in_triangle(point p, point t[3]) 
{
	point v0(t[2].x - t[0].x, t[2].y - t[0].y);
	point v1(t[1].x - t[0].x, t[1].y - t[0].y);
	point v2(p.x - t[0].x, p.y - t[0].y);

	int dot00 = t[0].x * t[0].x + t[0].y * t[0].y;
	int dot01 = t[0].x * t[1].x + t[0].y * t[1].y;
	int dot02 = t[0].x * t[2].x + t[0].y * t[2].y;
	int dot11 = t[1].x * t[1].x + t[1].y * t[1].y;
	int dot12 = t[1].x * t[2].x + t[1].y * t[2].y;
	float invDenom = 1 / float(dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
	return u >= 0.0f && v >= 0.0f && (u+v) < 1.0f;
}
}

FUNCTION_DEF(hex_get_tile_at, 3, 3, "hex_get_tile_at(hexmap, x, y) -> hex_tile object: Finds the hex tile at the given level co-ordinates")
	// Because we assume hexes are placed at a regular series of intervals
	variant v = args()[0]->evaluate(variables);
	hex::hex_map_ptr hexmap = hex::hex_map_ptr(v.try_convert<hex::hex_map>());
	ASSERT_LOG(hexmap, "hexmap not of the correct type.");
	const int mx = args()[1]->evaluate(variables).as_int();
	const int my = args()[2]->evaluate(variables).as_int();

	return variant(hexmap->get_tile_from_pixel_pos(mx, my).get());
END_FUNCTION_DEF(hex_get_tile_at)

FUNCTION_DEF(pixel_to_tile_coords, 1, 2, "pixel_to_tile_coords(args) -> [x,y]: Gets the tile at the pixel position given in the arguments. The position"
	"can either be a single list of two values suck as [x,y] or two seperate x,y co-ordinates.")
	int x, y;
	if(args().size() == 1) {
		variant vl = args()[0]->evaluate(variables);
		ASSERT_LOG(vl.is_list() && vl.num_elements() == 2, "Single argument must be a list of two elements");
		x = vl[0].as_int();
		y = vl[1].as_int();
	} else {
		x = args()[0]->evaluate(variables).as_int();
		y = args()[1]->evaluate(variables).as_int();
	}
	point xy = hex::hex_map::get_tile_pos_from_pixel_pos(x,y);
	std::vector<variant> v;
	v.push_back(variant(xy.x));
	v.push_back(variant(xy.y));
	return variant(&v);
END_FUNCTION_DEF(pixel_to_tile_coords)

FUNCTION_DEF(tile_to_pixel_coords, 2, 3, "tile_to_pixel_coords(x, y, (opt)string) -> [x,y]: Gets the center pixel co-ordinates of a given tile co-ordinate."
	"string can be effect the co-ordinates returned. \"bounding\" -> [x,y,w,h] Bounding rect of the tile. \"center\" -> [x,y] center co-ordinates of the tile(default)"
	"\"hex\" -> [[x0,y0],[x1,y1],[x2,y2],[x3,y3],[x4,y4],[x5,y5]] Co-ordinates of points around outside of the tile.")
	const int x = args()[0]->evaluate(variables).as_int();
	const int y = args()[1]->evaluate(variables).as_int();
	point p(hex::hex_map::get_pixel_pos_from_tile_pos(x, y));
	std::vector<variant> v;
	const int HexTileSize = 72;
	if(args().size() > 2) {
		const std::string opt(args()[2]->evaluate(variables).as_string());
		if(opt == "bounding" || opt == "rect") {
			v.push_back(variant(p.x));
			v.push_back(variant(p.y));
			v.push_back(variant(HexTileSize));
			v.push_back(variant(HexTileSize));
		} else if(opt == "hex") {
			const float angle = 2.0f * 3.14159265358979f / 6.0f;
			for(int i = 0; i < 6; i++) {
				v.push_back(variant(decimal(p.x + HexTileSize/2 + HexTileSize/2.0f * sin(i * angle))));
				v.push_back(variant(decimal(p.y + HexTileSize/2 + HexTileSize/2.0f * cos(i * angle))));
			}
		} else {
			v.push_back(variant(p.x + HexTileSize/2));
			v.push_back(variant(p.y + HexTileSize/2));
		}
		// unknown just drop down and do default
	} else {
		v.push_back(variant(p.x + HexTileSize/2));
		v.push_back(variant(p.y + HexTileSize/2));
	}
	return variant(&v);
END_FUNCTION_DEF(tile_to_pixel_coords)

FUNCTION_DEF(hex_pixel_coords, 2, 2, "hex_pixel_coords(x,y) -> [x,y]: Converts a pair of pixel co-ordinates to the corresponding tile co-ordinate.")
	const int x = args()[0]->evaluate(variables).as_int();
	const int y = args()[1]->evaluate(variables).as_int();
	point p(hex::hex_map::get_tile_pos_from_pixel_pos(x, y));
	std::vector<variant> v;
	v.push_back(variant(p.x));
	v.push_back(variant(p.y));
	return variant(&v);
END_FUNCTION_DEF(hex_pixel_coords)

FUNCTION_DEF(hex_location, 3, 3, "hex_location(x,y,string dir) -> [x,y]: calculates the co-ordinates of the tile in the given direction.")
	const int x = args()[0]->evaluate(variables).as_int();
	const int y = args()[1]->evaluate(variables).as_int();
	variant d = args()[2]->evaluate(variables);
	point p(x,y);
	if(d.is_list()) {
		for(int i = 0; i < d.num_elements(); i++) {
			p = hex::hex_map::loc_in_dir(p.x, p.y, d[i].as_string());
		}
	} else if(d.is_string()) {
		const std::string dir(d.as_string());
		p = hex::hex_map::loc_in_dir(x, y, dir);
	}
	std::vector<variant> v;
	v.push_back(variant(p.x));
	v.push_back(variant(p.y));
	return variant(&v);
END_FUNCTION_DEF(hex_location)

FUNCTION_DEF(hex_get_tile, 1, 1, "hex_get_tile(string) -> hex_tile object: Returns a hex tile object with the given name.")
	const std::string& tstr(args()[0]->evaluate(variables).as_string());
	return variant(hex::hex_object::get_hex_tile(tstr).get());
END_FUNCTION_DEF(hex_get_tile)

FUNCTION_DEF(hex_get_random_tile, 1, 2, "hex_get_random_tile(regex, (opt)count) -> hex_tile object(s): Generates either a single random tile or an array of count random tiles, picked from the given regular expression")
	const boost::regex re(args()[0]->evaluate(variables).as_string());
	std::vector<hex::hex_tile_ptr>& tile_list = hex::hex_object::get_editor_tiles();
	std::vector<hex::hex_tile_ptr> matches;
	for(size_t i = 0; i < tile_list.size(); ++i) {
		if(boost::regex_match(tile_list[i]->get_editor_info().type, re)) {
			matches.push_back(tile_list[i]);
		}
	}
	if(matches.empty()) {
		return variant();
	}
	if(args().size() > 1) {
		const int count = args()[1]->evaluate(variables).as_int();
		std::vector<variant> v;
		for(int i = 0; i < count; ++i ) {
			v.push_back(variant(matches[rand() % matches.size()].get()));
		}
		return variant(&v);
	} else {
		return variant(matches[rand() % matches.size()].get());
	}
END_FUNCTION_DEF(hex_get_random_tile)

FUNCTION_DEF(sha1, 1, 1, "sha1(string) -> string: Returns the sha1 hash of the given string")
	variant v = args()[0]->evaluate(variables);
	const std::string& s = v.as_string();
	boost::uuids::detail::sha1 hash;
	hash.process_bytes(s.c_str(), s.length());
	unsigned int digest[5];
	hash.get_digest(digest);
	std::stringstream str;
	for(int n = 0; n < 5; ++n) {
		str << std::hex << std::setw(8) << std::setfill('0') << digest[n];
	}
	return variant(str.str());
END_FUNCTION_DEF(sha1)

FUNCTION_DEF(get_module_args, 0, 0, "get_module_args() -> callable: Returns the current module callable environment")
	formula::fail_if_static_context();
	return variant(module::get_module_args().get());
END_FUNCTION_DEF(get_module_args)

FUNCTION_DEF(seed_rng, 0, 0, "seed_rng() -> none: Seeds the peudo-RNG used.")
	formula::fail_if_static_context();
	::srand(::time(NULL));
	return variant();
END_FUNCTION_DEF(seed_rng)

FUNCTION_DEF(lower, 1, 1, "lower(s) -> string: lowercase version of string")
	std::string s = args()[0]->evaluate(variables).as_string();
	boost::algorithm::to_lower(s);
	return variant(s);
END_FUNCTION_DEF(lower)

namespace {
void run_expression_for_edit_and_continue(expression_ptr expr, const game_logic::formula_callable* variables, bool* success)
{
	*success = false;
	expr->evaluate(*variables);
	*success = true;
}
}

FUNCTION_DEF(edit_and_continue, 2, 2, "edit_and_continue(expr, filename)")
	if(!preferences::edit_and_continue()) {
		return args()[0]->evaluate(variables);
	}

	const std::string filename = args()[1]->evaluate(variables).as_string();

	try {
		assert_recover_scope scope;
		return args()[0]->evaluate(variables);
	} catch (validation_failure_exception& e) {
		bool success = false;
		boost::function<void()> fn(boost::bind(run_expression_for_edit_and_continue, args()[0], &variables, &success));

		edit_and_continue_fn(filename, e.msg, fn);
		if(success == false) {
			_exit(0);
		}

		return args()[0]->evaluate(variables);
	}
END_FUNCTION_DEF(edit_and_continue)

class console_output_to_screen_command : public game_logic::command_callable
{
	bool value_;
public:
	explicit console_output_to_screen_command(bool value) : value_(value)
	{}

	virtual void execute(game_logic::formula_callable& ob) const 
	{
		debug_console::enable_screen_output(value_);
	}
};

FUNCTION_DEF(console_output_to_screen, 1, 1, "console_output_to_screen(bool) -> none: Turns the console output to the screen on and off")
	formula::fail_if_static_context();
	return variant(new console_output_to_screen_command(args()[0]->evaluate(variables).as_bool()));
END_FUNCTION_DEF(console_output_to_screen)

FUNCTION_DEF(user_preferences_path, 0, 0, "user_preferences_path() -> string: Returns the users preferences path")
	return variant(preferences::user_data_path());
END_FUNCTION_DEF(user_preferences_path)

class set_user_details_command : public game_logic::command_callable
{
	std::string username_;
	std::string password_;
public:
	explicit set_user_details_command(const std::string& username, const std::string& password) 
		: username_(username), password_(password)
	{}
	virtual void execute(game_logic::formula_callable& ob) const 
	{
		preferences::set_username(username_);
		if(password_.empty() == false) {
			preferences::set_password(password_);
		}
	}
};

FUNCTION_DEF(set_user_details, 1, 2, "set_user_details(string username, (opt) string password) -> none: Sets the username and password in the preferences.")
	formula::fail_if_static_context();
	return variant(new set_user_details_command(args()[0]->evaluate(variables).as_string(),
		args().size() > 1 ? args()[1]->evaluate(variables).as_string() : ""));
END_FUNCTION_DEF(set_user_details)

class set_cookie_command : public game_logic::command_callable
{
	variant cookie_;
public:
	explicit set_cookie_command(const variant& cookie) 
		: cookie_(cookie)
	{}
	virtual void execute(game_logic::formula_callable& ob) const 
	{
		preferences::set_cookie(cookie_);
	}
};

FUNCTION_DEF(set_cookie, 1, 1, "set_cookie(data) -> none: Sets the preferences user_data")
	formula::fail_if_static_context();
	return variant(new set_cookie_command(args()[0]->evaluate(variables)));
END_FUNCTION_DEF(set_cookie)

FUNCTION_DEF(get_cookie, 0, 0, "get_cookie() -> none: Returns the preferences user_data")
	formula::fail_if_static_context();
	return preferences::get_cookie();
END_FUNCTION_DEF(get_cookie)

FUNCTION_DEF(typeof, 1, 1, "typeof(expression) -> string: yields the statically known type of the given expression")
	variant_type_ptr type = args()[0]->query_variant_type();
	ASSERT_LOG(type.get() != NULL, "NULL VALUE RETURNED FROM TYPE QUERY");
	return variant(type->to_string());
END_FUNCTION_DEF(typeof)

}


UNIT_TEST(modulo_operation) {
	CHECK(game_logic::formula(variant("mod(-5, 20)")).execute() == game_logic::formula(variant("15")).execute(), "test failed");
	CHECK(game_logic::formula(variant("mod(-25, 20)")).execute() == game_logic::formula(variant("15")).execute(), "test failed");
	CHECK(game_logic::formula(variant("mod(15, 20)")).execute() == game_logic::formula(variant("15")).execute(), "test failed");
	CHECK(game_logic::formula(variant("mod(35, 20)")).execute() == game_logic::formula(variant("15")).execute(), "test failed");
}

UNIT_TEST(flatten_function) {
	CHECK(game_logic::formula(variant("flatten([1,[2,3]])")).execute() == game_logic::formula(variant("[1,2,3]")).execute(), "test failed");
	CHECK(game_logic::formula(variant("flatten([1,2,3,[[4,5],6]])")).execute() == game_logic::formula(variant("[1,2,3,4,5,6]")).execute(), "test failed");
	CHECK(game_logic::formula(variant("flatten([[1,2,3,4],5,6])")).execute() == game_logic::formula(variant("[1,2,3,4,5,6]")).execute(), "test failed");
	CHECK(game_logic::formula(variant("flatten([[[0,2,4],6,8],10,[12,14]])")).execute() == game_logic::formula(variant("[0,2,4,6,8,10,12,14]")).execute(), "test failed");
}

UNIT_TEST(sqrt_function) {
	CHECK_EQ(game_logic::formula(variant("sqrt(2147483)")).execute().as_int(), 1465);	

	for(uint64_t n = 0; n < 100000; n += 1000) {
		CHECK_EQ(game_logic::formula(variant(formatter() << "sqrt(" << n << ".0^2)")).execute().as_decimal(), decimal::from_int(n));
	}
}

UNIT_TEST(map_function) {
	CHECK_EQ(game_logic::formula(variant("map([2,3,4], value+index)")).execute(), game_logic::formula(variant("[2,4,6]")).execute());
}

UNIT_TEST(where_scope_function) {
	CHECK(game_logic::formula(variant("{'val': num} where num = 5")).execute() == game_logic::formula(variant("{'val': 5}")).execute(), "map where test failed");
	CHECK(game_logic::formula(variant("'five: ${five}' where five = 5")).execute() == game_logic::formula(variant("'five: 5'")).execute(), "string where test failed");
}

BENCHMARK(map_function) {
	using namespace game_logic;

	static map_formula_callable* callable = NULL;
	static variant callable_var;
	static variant main_callable_var;
	static std::vector<variant> v;
	
	if(callable == NULL) {
		callable = new map_formula_callable;
		callable_var = variant(callable);
		callable->add("x", variant(0));
		for(int n = 0; n != 1000; ++n) {
			v.push_back(callable_var);
		}

		callable = new map_formula_callable;
		main_callable_var = variant(callable);
		callable->add("items", variant(&v));
	}

	static formula f(variant("map(items, 'obj', 0)"));
	BENCHMARK_LOOP {
		f.execute(*callable);
	}
}

namespace game_logic {

const_formula_callable_definition_ptr get_map_callable_definition(const_formula_callable_definition_ptr base_def, variant_type_ptr key_type, variant_type_ptr value_type, const std::string& value_name)
{
	return const_formula_callable_definition_ptr(new map_callable_definition(base_def, key_type, value_type, value_name));
}

const_formula_callable_definition_ptr get_variant_comparator_definition(const_formula_callable_definition_ptr base_def, variant_type_ptr type)
{
	return const_formula_callable_definition_ptr(new variant_comparator_definition(base_def, type));
}
}
