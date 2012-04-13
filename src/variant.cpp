#include <cmath>
#include <set>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#include "boost/lexical_cast.hpp"

#include "asserts.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_callable_utils.hpp"
#include "i18n.hpp"
#include "unit_test.hpp"
#include "variant.hpp"
#include "wml_formula_callable.hpp"

namespace {
std::set<variant*> callable_variants_loading;

std::string variant_type_to_string(variant::TYPE type) {
	switch(type) {
	case variant::TYPE_NULL: 
		return "null";
	case variant::TYPE_BOOL: 
		return "bool";
	case variant::TYPE_INT: 
		return "int";
	case variant::TYPE_DECIMAL: 
		return "decimal";
	case variant::TYPE_CALLABLE: 
		return "object";
	case variant::TYPE_CALLABLE_LOADING: 
		return "object_loading";
	case variant::TYPE_LIST: 
		return "list";
	case variant::TYPE_STRING: 
		return "string";
	case variant::TYPE_MAP: 
		return "map";
	case variant::TYPE_FUNCTION: 
		return "function";
	default:
		assert(false);
		return "invalid";
	}
}

std::vector<const game_logic::formula_expression*> call_stack;

variant last_failed_query_map, last_failed_query_key;
variant last_query_map;
variant UnfoundInMapNullVariant;
}

void swap_variants_loading(std::set<variant*>& v)
{
	callable_variants_loading.swap(v);
}

void push_call_stack(const game_logic::formula_expression* frame)
{
	call_stack.push_back(frame);
}

void pop_call_stack()
{
	call_stack.pop_back();
}

std::string get_call_stack()
{
	variant current_frame;
	std::string res;
	std::vector<const game_logic::formula_expression*> reversed_call_stack = call_stack;
	std::reverse(reversed_call_stack.begin(), reversed_call_stack.end());
	for(std::vector<const game_logic::formula_expression*>::const_iterator i = reversed_call_stack.begin(); i != reversed_call_stack.end(); ++i) {
		const game_logic::formula_expression* p = *i;
		if(p && p->parent_formula() != current_frame) {
			current_frame = p->parent_formula();
			const variant::debug_info* info = current_frame.get_debug_info();
			if(!info) {
				res += "(UNKNOWN LOCATION)\n";
			} else {
				res += p->debug_pinpoint_location() + "\n";
			}
		}
	}

	return res;
}

std::string get_full_call_stack()
{
	std::string res;
	for(std::vector<const game_logic::formula_expression*>::const_iterator i = call_stack.begin();
	    i != call_stack.end(); ++i) {
		if(!*i) {
			continue;
		}
		res += formatter() << "  FRAME " << (i - call_stack.begin()) << ": " << (*i)->str() << "\n";
	}
	return res;
}

void output_formula_error_info();

type_error::type_error(const std::string& str) : message(str) {
	if(call_stack.empty() == false && call_stack.back()) {
		message += "\n" + call_stack.back()->debug_pinpoint_location();
	}

	std::cerr << "ERROR: " << message << "\n" << get_call_stack();
	output_formula_error_info();
}

struct variant_list {

	variant_list() : begin(elements.begin()), end(elements.end()),
	                 refcount(0), storage(NULL)
	{}

	variant_list(const variant_list& o) : info(o.info),
	   elements(o.begin, o.end), begin(elements.begin()), end(elements.end()),
	   refcount(1), storage(NULL)
	{}

	const variant_list& operator=(const variant_list& o) {
		info = o.info;
		elements.assign(o.begin, o.end),
		begin = elements.begin();
		end = elements.end();
		storage = NULL;
		return *this;
	}

	~variant_list() {
		if(storage && --storage->refcount == 0) {
			delete storage;
		}
	}

	size_t size() const { return end - begin; }

	variant::debug_info info;
	std::vector<variant> elements;
	std::vector<variant>::iterator begin, end;
	int refcount;
	variant_list* storage;
};

struct variant_string {
	variant::debug_info info;

	variant_string() : refcount(0)
	{}
	std::string str, translated_from;
	int refcount;
};

struct variant_map {
	variant::debug_info info;

	variant_map() : refcount(0)
	{}
	std::map<variant,variant> elements;
	int refcount;
};

struct variant_fn {
	variant_fn() : refcount(0)
	{}

	const std::string* begin_args;
	const std::string* end_args;

	game_logic::const_formula_ptr fn;

	game_logic::const_formula_callable_ptr callable;

	int base_slot;
	int refcount;
};

void variant::increment_refcount()
{
	switch(type_) {
	case TYPE_LIST:
		++list_->refcount;
		break;
	case TYPE_STRING:
		++string_->refcount;
		break;
	case TYPE_MAP:
		++map_->refcount;
		break;
	case TYPE_CALLABLE:
		intrusive_ptr_add_ref(callable_);
		break;
	case TYPE_CALLABLE_LOADING:
		callable_variants_loading.insert(this);
		break;
	case TYPE_FUNCTION:
		++fn_->refcount;
		break;

	// These are not used here, add them to silence a compiler warning.
	case TYPE_NULL:
	case TYPE_INT :
	case TYPE_BOOL :
		break;
	}
}

void variant::release()
{
	switch(type_) {
	case TYPE_LIST:
		if(--list_->refcount == 0) {
			delete list_;
		}
		break;
	case TYPE_STRING:
		if(--string_->refcount == 0) {
			delete string_;
		}
		break;
	case TYPE_MAP:
		if(--map_->refcount == 0) {
			delete map_;
		}
		break;
	case TYPE_CALLABLE:
		intrusive_ptr_release(callable_);
		break;
	case TYPE_CALLABLE_LOADING:
		callable_variants_loading.erase(this);
		break;
	case TYPE_FUNCTION:
		if(--fn_->refcount == 0) {
			delete fn_;
		}
		break;

	// These are not used here, add them to silence a compiler warning.
	case TYPE_NULL:
	case TYPE_INT:
	case TYPE_BOOL:
		break;
	}
}

void variant::set_debug_info(const debug_info& info)
{
	switch(type_) {
	case TYPE_LIST:
	case TYPE_STRING:
	case TYPE_MAP:
		*debug_info_ = info;
		break;
	}
}

const variant::debug_info* variant::get_debug_info() const
{
	switch(type_) {
	case TYPE_LIST:
	case TYPE_STRING:
	case TYPE_MAP:
		if(debug_info_->filename) {
			return debug_info_;
		}
		break;
	}

	return NULL;
}

variant::variant(const game_logic::formula_callable* callable)
	: type_(TYPE_CALLABLE), callable_(callable)
{
	if(callable == NULL) {
		type_ = TYPE_NULL;
		return;
	}
	increment_refcount();
}

variant::variant(std::vector<variant>* array)
    : type_(TYPE_LIST)
{
	assert(array);
	list_ = new variant_list;
	list_->elements.swap(*array);
	list_->begin = list_->elements.begin();
	list_->end = list_->elements.end();
	increment_refcount();
}

variant::variant(const char* s)
   : type_(TYPE_STRING)
{
	if(s == NULL) {
		type_ = TYPE_NULL;
		return;
	}
	string_ = new variant_string;
	string_->str = std::string(s);
	increment_refcount();
}

variant::variant(const std::string& str)
	: type_(TYPE_STRING)
{
	string_ = new variant_string;
	string_->str = str;
	increment_refcount();
}

variant variant::create_translated_string(const std::string& str)
{
	variant v(i18n::tr(str));
	v.string_->translated_from = str;
	return v;
}

variant::variant(std::map<variant,variant>* map)
    : type_(TYPE_MAP)
{
	for(std::map<variant, variant>::const_iterator i = map->begin(); i != map->end(); ++i) {
		if(i->first.is_bool()) {
			std::cerr << "VALUE: " << i->second.to_debug_string() << "\n";
			assert(false);
		}
	}

	assert(map);
	map_ = new variant_map;
	map_->elements.swap(*map);
	increment_refcount();
}

variant::variant(game_logic::const_formula_ptr fml, const std::vector<std::string>& args, const game_logic::formula_callable& callable, int base_slot)
  : type_(TYPE_FUNCTION)
{
	fn_ = new variant_fn;
	if(args.empty()) {
		fn_->begin_args = fn_->end_args = NULL;
	} else {
		fn_->begin_args = &args[0];
		fn_->end_args = fn_->begin_args + args.size();
	}
	fn_->base_slot = base_slot;
	fn_->fn = fml;
	fn_->callable = &callable;
	increment_refcount();

	if(fml->str_var().get_debug_info()) {
		set_debug_info(*fml->str_var().get_debug_info());
	}
}

const variant& variant::operator=(const variant& v)
{
	if(&v != this) {
		if(type_ > TYPE_INT) {
			release();
		}

		type_ = v.type_;
		value_ = v.value_;
		if(type_ > TYPE_INT) {
			increment_refcount();
		}
	}
	return *this;
}

const variant& variant::operator[](size_t n) const
{
	if(type_ == TYPE_CALLABLE) {
		assert(n == 0);
		return *this;
	}

	must_be(TYPE_LIST);
	assert(list_);
	if(n >= list_->size()) {
		throw type_error(formatter() << "invalid index of " << n << " into " << write_json());
	}

	return list_->begin[n];
}

const variant& variant::operator[](const variant v) const
{
	if(type_ == TYPE_CALLABLE) {
		assert(v.as_int() == 0);
		return *this;
	}

	if(type_ == TYPE_MAP) {
		assert(map_);
		std::map<variant,variant>::const_iterator i = map_->elements.find(v);
		if (i == map_->elements.end())
		{
			last_failed_query_map = *this;
			last_failed_query_key = v;

			return UnfoundInMapNullVariant;
		}

		last_query_map = *this;
		return i->second;
	} else if(type_ == TYPE_LIST) {
		return operator[](v.as_int());
	} else {
		const debug_info* info = get_debug_info();
		std::string loc;
		if(info) {
			loc = formatter() << " at " << *info->filename << " " << info->line << " (column " << info->column << ")\n";
		}
		throw type_error(formatter() << "type error: " << " expected a list or a map but found " << variant_type_to_string(type_) << " (" << write_json() << ") " << loc);
	}	
}

const variant& variant::operator[](const std::string& key) const
{
	return (*this)[variant(key)];
}

bool variant::has_key(const variant& key) const
{
	if(type_ != TYPE_MAP) {
		return false;
	}

	std::map<variant,variant>::const_iterator i = map_->elements.find(key);
	if(i != map_->elements.end() && i->second.is_null() == false) {
		return true;
	} else {
		return false;
	}
}

bool variant::has_key(const std::string& key) const
{
	return has_key(variant(key));
}

variant variant::get_keys() const
{
	must_be(TYPE_MAP);
	assert(map_);
	std::vector<variant> tmp;
	for(std::map<variant,variant>::const_iterator i=map_->elements.begin(); i != map_->elements.end(); ++i) {
			tmp.push_back(i->first);
	}
	return variant(&tmp);
}

variant variant::get_values() const
{
	must_be(TYPE_MAP);
	assert(map_);
	std::vector<variant> tmp;
	for(std::map<variant,variant>::const_iterator i=map_->elements.begin(); i != map_->elements.end(); ++i) {
			tmp.push_back(i->second);
	}
	return variant(&tmp);
}

size_t variant::num_elements() const
{
	if (type_ == TYPE_NULL){
		return 0;
	} else if(type_ == TYPE_CALLABLE) {
		return 1;
	} else if (type_ == TYPE_LIST) {
		assert(list_);
		return list_->size();
	} else if (type_ == TYPE_MAP) {
		assert(map_);
		return map_->elements.size();
	} else {
		const debug_info* info = get_debug_info();
		std::string loc;
		if(info) {
			loc = formatter() << " at " << *info->filename << " " << info->line << " (column " << info->column << ")\n";
		}
		throw type_error(formatter() << "type error: " << " expected a list or a map but found " << variant_type_to_string(type_) << " (" << write_json() << ")" << loc);
	}
}

variant variant::get_list_slice(int begin, int end) const
{
	std::vector<variant> items;
	variant result(&items);
	if(end <= begin) {
		return result;
	}

	must_be(TYPE_LIST);

	if(begin < 0 || end > list_->size()) {
		throw type_error(formatter() << "ILLEGAL INDEX INTO LIST WHEN SLICING: " << begin << ", " << end << " / " << list_->size());
	}

	result.list_->begin = list_->begin + begin;
	result.list_->end = list_->begin + end;
	result.list_->storage = list_;
	list_->refcount++;

	return result;
}

variant variant::operator()(const std::vector<variant>& args) const
{
	must_be(TYPE_FUNCTION);
	boost::intrusive_ptr<game_logic::slot_formula_callable> callable = new game_logic::slot_formula_callable;
	if(fn_->callable) {
		callable->set_fallback(fn_->callable);
	}

	callable->set_base_slot(fn_->base_slot);

	if(args.size() > fn_->end_args - fn_->begin_args) {
		throw type_error(formatter() << "Function passed " << args.size() << " arguments, " << (fn_->end_args - fn_->begin_args) << " expected");
	}

	for(size_t n = 0; n != args.size(); ++n) {
		callable->add(args[n]);
	}

	for(size_t n = args.size(); n < fn_->end_args - fn_->begin_args; ++n) {
		callable->add(variant());
	}

	return fn_->fn->execute(*callable);
}

variant variant::get_member(const std::string& str) const
{
	if(is_callable()) {
		return callable_->query_value(str);
	} else if(is_map()) {
		return (*this)[str];
	}

	if(str == "self") {
		return *this;
	} else {
		return variant();
	}
}

bool variant::as_bool(bool default_value) const
{
	switch(type_) {
	case TYPE_INT: return int_value_ != 0;
	case TYPE_BOOL: return bool_value_;
	default: return default_value;
	}
}

bool variant::as_bool() const
{
	bool default_value = false;
	switch(type_) {
	case TYPE_NULL:
		return default_value;
	case TYPE_BOOL:
		return bool_value_;
	case TYPE_INT:
		return int_value_ != 0;
	case TYPE_DECIMAL:
		return decimal_value_ != 0;
	case TYPE_CALLABLE_LOADING:
		return true;
	case TYPE_CALLABLE:
		return callable_ != NULL;
	case TYPE_LIST:
		return !list_->size() == 0;
	case TYPE_MAP:
		return !map_->elements.empty();
	case TYPE_STRING:
		return !string_->str.empty();
	case TYPE_FUNCTION:
		return true;
	default:
		assert(false);
		return false;
	}
}

std::vector<variant> variant::as_list() const
{
	if(is_list()) {
		if(list_->elements.empty() == false) {
			return list_->elements;
		} else {
			return std::vector<variant>(list_->begin, list_->end);
		}
	} else if(is_null()) {
		return std::vector<variant>();
	} else {
		std::vector<variant> v;
		v.push_back(*this);
		return v;
	}
}

std::vector<std::string> variant::as_list_string() const
{
	std::vector<std::string> result;
	must_be(TYPE_LIST);
	result.reserve(list_->size());
	for(int n = 0; n != list_->size(); ++n) {
		list_->begin[n].must_be(TYPE_STRING);
		result.push_back(list_->begin[n].as_string());
	}

	return result;
}

std::vector<int> variant::as_list_int() const
{
	std::vector<int> result;
	must_be(TYPE_LIST);
	result.reserve(list_->size());
	for(int n = 0; n != list_->size(); ++n) {
		result.push_back(list_->begin[n].as_int());
	}

	return result;
}

std::vector<decimal> variant::as_list_decimal() const
{
	std::vector<decimal> result;
	must_be(TYPE_LIST);
	result.reserve(list_->size());
	for(int n = 0; n != list_->size(); ++n) {
		result.push_back(list_->begin[n].as_decimal());
	}

	return result;
}

const std::map<variant,variant>& variant::as_map() const
{
	if(is_map()) {
		return map_->elements;
	} else {
		static std::map<variant,variant> EmptyMap;
		return EmptyMap;
	}
}

variant variant::add_attr(variant key, variant value)
{
	last_query_map = variant();

	if(is_map()) {
		if(map_->refcount > 1) {
			map_->refcount--;
			map_ = new variant_map(*map_);
			map_->refcount = 1;
		}

		make_unique();
		map_->elements[key] = value;
		return *this;
	} else {
		return variant();
	}
}

variant variant::remove_attr(variant key)
{
	last_query_map = variant();

	if(is_map()) {
		if(map_->refcount > 1) {
			map_->refcount--;
			map_ = new variant_map(*map_);
			map_->refcount = 1;
		}

		make_unique();
		map_->elements.erase(key);
		return *this;
	} else {
		return variant();
	}
}

void variant::add_attr_mutation(variant key, variant value)
{
	if(is_map()) {
		map_->elements[key] = value;
	}
}

void variant::remove_attr_mutation(variant key)
{
	if(is_map()) {
		map_->elements.erase(key);
	}
}

std::string variant::as_string_default(const char* default_value) const
{
	if(is_null()) {
		if(default_value) {
			return std::string(default_value);
		} else {
			return std::string();
		}
	}

	return as_string();
}

const std::string& variant::as_string() const
{
	must_be(TYPE_STRING);
	assert(string_);
	return string_->str;
}

variant variant::operator+(const variant& v) const
{
	if(type_ == TYPE_INT && v.type_ == TYPE_INT) {
		//strictly an optimization -- this is handled below, but the case
		//of adding two integers is the most common so we want it to be fast.
		return variant(int_value_ + v.int_value_);
	}

	if(type_ == TYPE_STRING) {
		if(v.type_ == TYPE_MAP) {
			return variant(as_string() + v.as_string());
		} else if(v.type_ == TYPE_STRING) {
			return variant(as_string() + v.as_string());
		}

		std::string s;
		v.serialize_to_string(s);
		return variant(as_string() + s);
	}

	if(v.type_ == TYPE_STRING) {
		std::string s;
		serialize_to_string(s);
		return variant(s + v.as_string());
	}
	if(type_ == TYPE_DECIMAL || v.type_ == TYPE_DECIMAL) {
		return variant(as_decimal() + v.as_decimal());
	}

	if(type_ == TYPE_INT) {
		return variant(int_value_ + v.as_int());
	}

	if(type_ == TYPE_BOOL) {
		return variant(as_int() + v.as_int());
	}

	if(type_ == TYPE_NULL) {
		return v;
	} else if(v.type_ == TYPE_NULL) {
		return *this;
	}

	if(type_ == TYPE_LIST) {
		if(v.type_ == TYPE_LIST) {
			std::vector<variant> res;
			res.reserve(list_->size() + v.list_->size());
			for(size_t i = 0; i < list_->size(); ++i) {
				const variant& var = list_->begin[i];
				res.push_back(var);
			}

			for(size_t j = 0; j < v.list_->size(); ++j) {
				const variant& var = v.list_->begin[j];
				res.push_back(var);
			}

			return variant(&res);
		}
	}
	if(type_ == TYPE_MAP) {
		if(v.type_ == TYPE_MAP) {
			std::map<variant,variant> res(map_->elements);

			for(std::map<variant,variant>::const_iterator i = v.map_->elements.begin(); i != v.map_->elements.end(); ++i) {
				res[i->first] = i->second;
			}

			return variant(&res);
		}
	}

	return variant(as_int() + v.as_int());
}

variant variant::operator-(const variant& v) const
{
	if(type_ == TYPE_DECIMAL || v.type_ == TYPE_DECIMAL) {
		return variant(as_decimal() - v.as_decimal());
	}

	return variant(as_int() - v.as_int());
}

variant variant::operator*(const variant& v) const
{
	if(type_ == TYPE_DECIMAL || v.type_ == TYPE_DECIMAL) {
		return variant(as_decimal() * v.as_decimal());
	}

	if(type_ == TYPE_LIST) {
		int ncopies = v.as_int();
		if(ncopies < 0) {
			ncopies *= -1;
		}
		std::vector<variant> res;
		res.reserve(list_->size()*ncopies);
		for(int n = 0; n != ncopies; ++n) {
			for(int m = 0; m != list_->size(); ++m) {
				res.push_back(list_->begin[m]);
			}
		}

		return variant(&res);
	}
	
	return variant(as_int() * v.as_int());
}

variant variant::operator/(const variant& v) const
{
	if(type_ == TYPE_DECIMAL || v.type_ == TYPE_DECIMAL) {
		if(v.as_decimal().value() == 0) {
			throw type_error((formatter() << "divide by zero error").str());
		}

		return variant(as_decimal() / v.as_decimal());
	}

	const int numerator = as_int();
	const int denominator = v.as_int();
	if(denominator == 0) {
		throw type_error(formatter() << "divide by zero error");
	}

	return variant(numerator/denominator);
}

variant variant::operator%(const variant& v) const
{
	const int numerator = as_int();
	const int denominator = v.as_int();
	if(denominator == 0) {
		throw type_error(formatter() << "divide by zero error");
	}

	return variant(numerator%denominator);
}

variant variant::operator^(const variant& v) const
{
	if( type_ == TYPE_DECIMAL || v.type_ == TYPE_DECIMAL ) {
		double res = pow( as_decimal().value()/double(VARIANT_DECIMAL_PRECISION),
		                v.as_decimal().value()/double(VARIANT_DECIMAL_PRECISION));		
		res *= DECIMAL_PRECISION;
#if defined(TARGET_BLACKBERRY)
		return variant(static_cast<int64_t>(llround(res)), DECIMAL_VARIANT);
#else
		return variant(static_cast<int64_t>(res), DECIMAL_VARIANT);
#endif
	}

	return variant(static_cast<int>(pow(static_cast<double>(as_int()), v.as_int())));
}

variant variant::operator-() const
{
	if(type_ == TYPE_DECIMAL) {
		return variant(-decimal_value_, variant::DECIMAL_VARIANT);
	}

	return variant(-as_int());
}

bool variant::operator==(const variant& v) const
{
	if(type_ != v.type_) {
		if(type_ == TYPE_DECIMAL || v.type_ == TYPE_DECIMAL) {
			return as_decimal() == v.as_decimal();
		}

		return false;
	}

	switch(type_) {
	case TYPE_NULL: {
		return v.is_null();
	}

	case TYPE_STRING: {
		return string_->str == v.string_->str;
	}

	case TYPE_BOOL: {
		return bool_value_ == v.bool_value_;
	}

	case TYPE_INT: {
		return int_value_ == v.int_value_;
	}

	case TYPE_DECIMAL: {
		return decimal_value_ == v.decimal_value_;
	}

	case TYPE_LIST: {
		if(num_elements() != v.num_elements()) {
			return false;
		}

		for(size_t n = 0; n != num_elements(); ++n) {
			if((*this)[n] != v[n]) {
				return false;
			}
		}

		return true;
	}

	case TYPE_MAP: {
		return map_->elements == v.map_->elements;
	}

	case TYPE_CALLABLE_LOADING: {
		return false;
	}

	case TYPE_CALLABLE: {
		return callable_->equals(v.callable_);
	}
	case TYPE_FUNCTION: {
		return fn_ == v.fn_;
	}
	}

	assert(false);
	return false;
}

bool variant::operator!=(const variant& v) const
{
	return !operator==(v);
}

bool variant::operator<=(const variant& v) const
{
	if(type_ != v.type_) {
		if(type_ == TYPE_DECIMAL || v.type_ == TYPE_DECIMAL) {
			return as_decimal() <= v.as_decimal();
		}

		return type_ < v.type_;
	}

	switch(type_) {
	case TYPE_NULL: {
		return true;
	}

	case TYPE_STRING: {
		return string_->str <= v.string_->str;
	}

	case TYPE_BOOL: {
		return bool_value_ <= v.bool_value_;
	}

	case TYPE_INT: {
		return int_value_ <= v.int_value_;
	}

	case TYPE_DECIMAL: {
		return decimal_value_ <= v.decimal_value_;
	}

	case TYPE_LIST: {
		for(size_t n = 0; n != num_elements() && n != v.num_elements(); ++n) {
			if((*this)[n] < v[n]) {
				return true;
			} else if((*this)[n] > v[n]) {
				return false;
			}
		}

		return num_elements() <= v.num_elements();
	}

	case TYPE_MAP: {
		return map_->elements <= v.map_->elements;
	}

	case TYPE_CALLABLE_LOADING: {
		return false;
	}

	case TYPE_CALLABLE: {
		return !v.callable_->less(callable_);
	}
	}

	assert(false);
	return false;
}

bool variant::operator>=(const variant& v) const
{
	return v <= *this;
}

bool variant::operator<(const variant& v) const
{
	return !(*this >= v);
}

bool variant::operator>(const variant& v) const
{
	return !(*this <= v);
}

void variant::throw_type_error(variant::TYPE t) const
{
	if(this == &UnfoundInMapNullVariant) {
		const debug_info* info = last_failed_query_map.get_debug_info();
		if(info) {
			throw type_error(formatter() << "In object at " << *info->filename << " " << info->line << " (column " << info->column << ") did not find attribute " << last_failed_query_key << " which was expected to be a " << variant_type_to_string(t));
		}
	}

	if(last_query_map.is_map() && last_query_map.get_debug_info()) {
		for(std::map<variant,variant>::const_iterator i = last_query_map.map_->elements.begin(); i != last_query_map.map_->elements.end(); ++i) {
			if(this == &i->second) {
				const debug_info* info = i->first.get_debug_info();
				if(info == NULL) {
					info = last_query_map.get_debug_info();
				}
				throw type_error(formatter() << "In object at " << *info->filename << " " << info->line << " (column " << info->column << ") attribute for " << i->first << " was " << *this << ", which is a " << variant_type_to_string(type_) << ", must be a " << variant_type_to_string(t));
				
			}
		}
	}

	const debug_info* info = get_debug_info();
	std::string loc;
	if(info) {
		loc = formatter() << " at " << *info->filename << " " << info->line << " (column " << info->column << "\n";
	}

	throw type_error(formatter() << "type error: " << " expected " << variant_type_to_string(t) << " but found " << variant_type_to_string(type_) << " " << write_json() << loc);
}

void variant::serialize_to_string(std::string& str) const
{
	switch(type_) {
	case TYPE_NULL:
		str += "null()";
		break;
	case TYPE_BOOL:
		str += bool_value_ ? "true()" : "false()";
		break;
	case TYPE_INT:
		str += boost::lexical_cast<std::string>(int_value_);
		break;
	case TYPE_DECIMAL: {
		std::ostringstream s;
		s << decimal(decimal_value_);
		str += s.str();
		break;
	}
	case TYPE_CALLABLE_LOADING: {
		ASSERT_LOG(false, "TRIED TO SERIALIZE A VARIANT LOADING");
		break;
	}
	case TYPE_CALLABLE: {
		const game_logic::wml_serializable_formula_callable* obj = try_convert<game_logic::wml_serializable_formula_callable>();
		if(obj) {
			//we have an object that is to be serialized into WML. However,
			//it might be present in the level or a reference to it held
			//from multiple objects. So we record the address of it and
			//register it to be recorded seperately.
			char buf[256];
			sprintf(buf, "deserialize('%p')", obj);
			str += buf;
			return;
		}

		callable_->serialize(str);
		break;
	}
	case TYPE_LIST: {
		str += "[";
		bool first_time = true;
		for(size_t i=0; i < list_->size(); ++i) {
			const variant& var = list_->begin[i];
			if(!first_time) {
				str += ",";
			}
			first_time = false;
			var.serialize_to_string(str);
		}
		str += "]";
		break;
	}
	case TYPE_MAP: {
		str += "{";
		bool first_time = true;
		for(std::map<variant,variant>::const_iterator i=map_->elements.begin(); i != map_->elements.end(); ++i) {
			if(!first_time) {
				str += ",";
			}
			first_time = false;
			i->first.serialize_to_string(str);
			str += ": ";
			i->second.serialize_to_string(str);
		}
		str += "}";
		break;
	}
	case TYPE_STRING: {
		if( !string_->str.empty() ) {
			if(string_->str[0] == '~' && string_->str[string_->str.length()-1] == '~') {
				str += string_->str;
			} else {
				const char* delim = "'";
				if(strchr(string_->str.c_str(), '\'')) {
					delim = "~";
				}

				str += delim;
				str += string_->str;
				str += delim;
			}
		}
		break;
	}
	case TYPE_FUNCTION:
		fprintf(stderr, "ATTEMPT TO SERIALIZE FUNCTION: %s\n", fn_->fn->str().c_str());
		assert(false);
	default:
		assert(false);
	}
}

void variant::serialize_from_string(const std::string& str)
{
	try {
		*this = game_logic::formula(variant(str)).execute();
	} catch(...) {
		*this = variant(str);
	}
}

variant variant::create_variant_under_construction(intptr_t id)
{
	variant v;
	v.type_ = TYPE_CALLABLE_LOADING;
	v.callable_loading_ = id;
	v.increment_refcount();
	return v;
}

int variant::refcount() const
{

	switch(type_) {
	case TYPE_LIST:
		return list_->refcount;
		break;
	case TYPE_STRING:
		return string_->refcount;
		break;
	case TYPE_MAP:
		return map_->refcount;
		break;
	case TYPE_CALLABLE:
		return callable_->refcount();
		break;
	default:
		return -1;
	}
}

void variant::make_unique()
{
	if(refcount() == 1) {
		return;
	}

	switch(type_) {
	case TYPE_LIST: {
		list_->refcount--;
		list_ = new variant_list(*list_);
		foreach(variant& v, list_->elements) {
			v.make_unique();
		}
		break;
	}
	case TYPE_STRING:
		string_->refcount--;
		string_ = new variant_string(*string_);
		string_->refcount = 1;
		break;
	case TYPE_MAP: {
		std::map<variant,variant> m;
		for(std::map<variant,variant>::const_iterator i = map_->elements.begin(); i != map_->elements.end(); ++i) {
			variant key = i->first;
			variant value = i->second;
			key.make_unique();
			value.make_unique();
			m[key] = value;
		}

		map_->refcount--;

		variant_map* vm = new variant_map;
		vm->info = map_->info;
		vm->refcount = 1;
		vm->elements.swap(m);
		map_ = vm;
		break;
	}
	default:
		break;
	}
}

std::string variant::string_cast() const
{
	switch(type_) {
	case TYPE_NULL:
		return "0";
	case TYPE_BOOL:
		return bool_value_ ? "true" : "false";
	case TYPE_INT:
		return boost::lexical_cast<std::string>(int_value_);
	case TYPE_DECIMAL: {
		std::string res;
		serialize_to_string(res);
		return res;
	}
	case TYPE_CALLABLE_LOADING:
		return "(object loading)";
	case TYPE_CALLABLE:
		return "(object)";
	case TYPE_LIST: {
		std::string res = "";
		for(size_t i=0; i < list_->size(); ++i) {
			const variant& var = list_->begin[i];
			if(!res.empty()) {
				res += ", ";
			}

			res += var.string_cast();
		}

		return res;
	}
	case TYPE_MAP: {
		std::string res = "";
		for(std::map<variant,variant>::const_iterator i=map_->elements.begin(); i != map_->elements.end(); ++i) {
			if(!res.empty()) {
				res += ",";
			}
			res += i->first.string_cast();
			res += ": ";
			res += i->second.string_cast();
		}
		return res;
	}

	case TYPE_STRING:
		return string_->str;
	default:
		assert(false);
		return "invalid";
	}
}

std::string variant::to_debug_string(std::vector<const game_logic::formula_callable*>* seen) const
{
	std::vector<const game_logic::formula_callable*> seen_stack;
	if(!seen) {
		seen = &seen_stack;
	}

	std::ostringstream s;
	switch(type_) {
	case TYPE_NULL:
		s << "(null)";
	case TYPE_BOOL:
		s << (bool_value_ ? "true" : "false");
		break;
	case TYPE_INT:
		s << int_value_;
		break;
	case TYPE_DECIMAL:
		s << string_cast();
		break;
	case TYPE_LIST: {
		s << "[";
		for(size_t n = 0; n != num_elements(); ++n) {
			if(n != 0) {
				s << ", ";
			}

			s << operator[](n).to_debug_string(seen);
		}
		s << "]";
		break;
	}
	case TYPE_CALLABLE_LOADING: {
		char buf[64];
		sprintf(buf, "(loading %x)", callable_loading_);
		s << buf;
	}

	case TYPE_CALLABLE: {
		char buf[64];
		sprintf(buf, "(%p)", callable_);
		s << buf << "{";
		if(std::find(seen->begin(), seen->end(), callable_) == seen->end()) {
			seen->push_back(callable_);
			std::vector<game_logic::formula_input> v = callable_->inputs();
			bool first = true;
			for(size_t i=0; i<v.size(); ++i) {
				const game_logic::formula_input& input = v[i];
				if(!first) {
					s << ",\n";
				}
				first = false;
				s << input.name << ": " << callable_->query_value(input.name).to_debug_string(seen);
			}
		} else {
			s << "...";
		}
		s << "}";
		break;
	}
	case TYPE_MAP: {
		s << "{";
		bool first_time = true;
		for(std::map<variant,variant>::const_iterator i=map_->elements.begin(); i != map_->elements.end(); ++i) {
			if(!first_time) {
				s << ",";
			}
			first_time = false;
			s << i->first.to_debug_string(seen);
			s << ": ";
			s << i->second.to_debug_string(seen);
		}
		s << "}";
		break;
	}
	case TYPE_STRING: {
		s << "'" << string_->str << "'";
		break;
	}
	}

	if(get_debug_info()) {
		s << " (" << get_debug_info()->message() << ")";
	}

	return s.str();
}

std::string variant::write_json(bool pretty) const
{
	std::ostringstream s;
	if(pretty) {
		write_json_pretty(s, "");
	} else {
		write_json(s);
	}
	return s.str();
}

void variant::write_json(std::ostream& s) const
{
	switch(type_) {
	case TYPE_NULL: {
		s << "null";
		return;
	}
	case TYPE_BOOL:
		s << (bool_value_ ? "true" : "false");
		break;
	case TYPE_INT: {
		s << as_int();
		return;
	}
	case TYPE_DECIMAL: {
		s << decimal(decimal_value_);
		return;
	}
	case TYPE_MAP: {
		s << "{";
		for(std::map<variant,variant>::const_iterator i = map_->elements.begin(); i != map_->elements.end(); ++i) {
			if(i != map_->elements.begin()) {
				s << ',';
			}
			s << '"' << i->first.as_string() << "\":";
			i->second.write_json(s);
		}

		s << "}";
		return;
	}
	case TYPE_LIST: {
		s << "[";

		for(std::vector<variant>::const_iterator i = list_->begin;
		    i != list_->end; ++i) {
			if(i != list_->begin) {
				s << ',';
			}

			i->write_json(s);
		}

		s << "]";
		return;
	}
	case TYPE_STRING: {
		const std::string& str = string_->translated_from.empty() ? string_->str : string_->translated_from;
		const char delim = string_->translated_from.empty() ? '"' : '~';
		if(std::count(str.begin(), str.end(), '\\') || std::count(str.begin(), str.end(), delim)) {
			//escape the string
			s << delim;
			for(std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
				if(*i == '\\' || *i == delim) {
					s << '\\';
				}

				s << *i;
			}
			s << delim;
		} else {
			s << delim << string_->str << delim;
		}
		return;
	}
	case TYPE_CALLABLE: {
		std::string str;
		serialize_to_string(str);
		s << "\"@eval " << str << "\"";
		return;
	}
	default:
		throw type_error(formatter() << "illegal type to serialize to json: " << to_debug_string());
	}
}

void variant::write_json_pretty(std::ostream& s, std::string indent) const
{
	switch(type_) {
	case TYPE_MAP: {
		s << "{";
		indent += "\t";
		for(std::map<variant,variant>::const_iterator i = map_->elements.begin(); i != map_->elements.end(); ++i) {
			if(i != map_->elements.begin()) {
				s << ',';
			}
			s << "\n" << indent << '"' << i->first.string_cast() << "\": ";
			i->second.write_json_pretty(s, indent);
		}
		indent.resize(indent.size()-1);

		s << "\n" << indent << "}";
		return;
	}
	case TYPE_LIST: {
		bool found_non_scalar = false;
		for(std::vector<variant>::const_iterator i = list_->begin;
		    i != list_->end; ++i) {
			if(i->is_list() || i->is_map()) {
				found_non_scalar = true;
				break;
			}
		}

		if(!found_non_scalar) {
			write_json(s);
			return;
		}


		s << "[";

		indent += "\t";
		for(std::vector<variant>::const_iterator i = list_->begin;
		    i != list_->end; ++i) {
			if(i != list_->begin) {
				s << ',';
			}

			s << "\n" << indent;

			i->write_json_pretty(s, indent);
		}

		indent.resize(indent.size()-1);

		if(list_->size() > 0) {
			s << "\n" << indent << "]";
		} else {
			s << "]";
		}

		return;
	}

	default:
		write_json(s);
		break;
	}
}

std::string variant::debug_info::message() const
{
	std::ostringstream s;
	s << *filename << " " << line << " (column " << column << ")";
	return s.str();
}

std::ostream& operator<<(std::ostream& os, const variant& v)
{
	os << v.write_json();
	return os;
}

UNIT_TEST(variant_decimal)
{
	variant d(9876000, variant::DECIMAL_VARIANT);
	variant d2(4000, variant::DECIMAL_VARIANT);
	CHECK_EQ(d.as_decimal().value(), 9876000);
	CHECK_EQ(d.as_int(), 9);
	CHECK_EQ(d.string_cast(), "9.876");
	CHECK_EQ((d + d2).as_decimal().value(), 9880000);
}

BENCHMARK(variant_assign)
{
	variant v(4);
	std::vector<variant> vec(1000);
	BENCHMARK_LOOP {
		for(int n = 0; n != vec.size(); ++n) {
			vec[n] = v;
		}
	}
}
