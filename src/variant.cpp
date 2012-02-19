#include <cmath>
#include <set>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#include "boost/lexical_cast.hpp"

#include "asserts.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "unit_test.hpp"
#include "variant.hpp"
#include "wml_formula_callable.hpp"

namespace {
std::set<variant*> callable_variants_loading;

std::string variant_type_to_string(variant::TYPE type) {
	switch(type) {
	case variant::TYPE_NULL: 
		return "null";
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

std::vector<const char*> call_stack;
}

void swap_variants_loading(std::set<variant*>& v)
{
	callable_variants_loading.swap(v);
}

void push_call_stack(const char* str)
{
	call_stack.push_back(str);
}

void pop_call_stack()
{
	call_stack.pop_back();
}

std::string get_call_stack()
{
	std::string res;
	for(std::vector<const char*>::const_iterator i = call_stack.begin();
	    i != call_stack.end(); ++i) {
		if(!*i) {
			continue;
		}
		res += formatter() << "  FRAME " << (i - call_stack.begin()) << ": " << *i << "\n";
	}
	return res;
}

void output_formula_error_info();

type_error::type_error(const std::string& str) : message(str) {
	std::cerr << "ERROR: " << message << "\n" << get_call_stack();
	output_formula_error_info();
	assert(false);
}

struct variant_list {
	variant_list() : refcount(0)
	{}
	std::vector<variant> elements;
	int refcount;
};

struct variant_string {
	variant_string() : refcount(0)
	{}
	std::string str;
	int refcount;
};

struct variant_map {
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
		break;
	}
}

std::vector<variant>& variant::initialize_list()
{
	if(type_ == TYPE_LIST) {
		if(list_->refcount == 1) {
			list_->elements.clear();
			return list_->elements;
		}
		release();
	} else {
		release();
		type_ = TYPE_LIST;
	}

	list_ = new variant_list;
	increment_refcount();
	return list_->elements;
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
	case TYPE_INT :
		break;
	}
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
	increment_refcount();
}

variant::variant(const std::string& str)
	: type_(TYPE_STRING)
{
	string_ = new variant_string;
	string_->str = str;
	increment_refcount();
}

variant::variant(std::map<variant,variant>* map)
    : type_(TYPE_MAP)
{
	assert(map);
	map_ = new variant_map;
	map_->elements.swap(*map);
	increment_refcount();
}

variant::variant(game_logic::const_formula_ptr fml, const std::vector<std::string>& args, const game_logic::formula_callable& callable)
  : type_(TYPE_FUNCTION)
{
	fn_ = new variant_fn;
	if(args.empty()) {
		fn_->begin_args = fn_->end_args = NULL;
	} else {
		fn_->begin_args = &args[0];
		fn_->end_args = fn_->begin_args + args.size();
	}
	fn_->fn = fml;
	fn_->callable = &callable;
	increment_refcount();
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
	if(n >= list_->elements.size()) {
		throw type_error("invalid index");
	}

	return list_->elements[n];
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
			static variant null_variant;
			return null_variant;
		}
		return i->second;
	} else if(type_ == TYPE_LIST) {
		return operator[](v.as_int());
	} else {
		throw type_error(formatter() << "type error: " << " expected a list or a map but found " << variant_type_to_string(type_) << " (" << to_debug_string() << ")");
	}	
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
		return list_->elements.size();
	} else if (type_ == TYPE_MAP) {
		assert(map_);
		return map_->elements.size();
	} else {
		throw type_error(formatter() << "type error: " << " expected a list or a map but found " << variant_type_to_string(type_) << " (" << to_debug_string() << ")");
	}
}

variant variant::operator()(const std::vector<variant>& args) const
{
	must_be(TYPE_FUNCTION);
	game_logic::map_formula_callable* callable = new game_logic::map_formula_callable(fn_->callable.get());
	variant v(callable);

	for(size_t n = 0; n != args.size() && n != fn_->end_args - fn_->begin_args; ++n) {
		callable->add(fn_->begin_args[n], args[n]);
	}

	return fn_->fn->execute(*callable);
}

variant variant::get_member(const std::string& str) const
{
	if(is_callable()) {
		return callable_->query_value(str);
	}

	if(str == "self") {
		return *this;
	} else {
		return variant();
	}
}

bool variant::as_bool() const
{
	switch(type_) {
	case TYPE_NULL:
		return false;
	case TYPE_INT:
		return int_value_ != 0;
	case TYPE_DECIMAL:
		return decimal_value_ != 0;
	case TYPE_CALLABLE_LOADING:
		return true;
	case TYPE_CALLABLE:
		return callable_ != NULL;
	case TYPE_LIST:
		return !list_->elements.empty();
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

	if(type_ == TYPE_NULL) {
		return v;
	} else if(v.type_ == TYPE_NULL) {
		return *this;
	}

	if(type_ == TYPE_LIST) {
		if(v.type_ == TYPE_LIST) {
			std::vector<variant> res;
			res.reserve(list_->elements.size() + v.list_->elements.size());
			for(size_t i = 0; i<list_->elements.size(); ++i) {
				const variant& var = list_->elements[i];
				res.push_back(var);
			}

			for(size_t j = 0; j<v.list_->elements.size(); ++j) {
				const variant& var = v.list_->elements[j];
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
		const std::vector<variant>& items = list_->elements;
		std::vector<variant> res;
		res.reserve(items.size()*ncopies);
		for(int n = 0; n != ncopies; ++n) {
			for(int m = 0; m != items.size(); ++m) {
				res.push_back(items[m]);
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
		return variant(static_cast<int64_t>(res), DECIMAL_VARIANT);
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
	throw type_error(formatter() << "type error: " << " expected " << variant_type_to_string(t) << " but found " << variant_type_to_string(type_) << " (" << to_debug_string() << ")");
}

void variant::serialize_to_string(std::string& str) const
{
	switch(type_) {
	case TYPE_NULL:
		str += "null()";
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
		if(game_logic::wml_formula_callable_serialization_scope::is_active()) {
			const game_logic::wml_serializable_formula_callable* obj = try_convert<game_logic::wml_serializable_formula_callable>();
			if(obj) {
				//we have an object that is to be serialized into WML. However,
				//it might be present in the level or a reference to it held
				//from multiple objects. So we record the address of it and
				//register it to be recorded seperately.
				str += "deserialize('" + game_logic::wml_formula_callable_serialization_scope::require_serialized_object(obj) + "')";
				return;
			}
		}

		callable_->serialize(str);
		break;
	}
	case TYPE_LIST: {
		str += "[";
		bool first_time = true;
		for(size_t i=0; i<list_->elements.size(); ++i) {
			const variant& var = list_->elements[i];
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
			str += "->";
			i->second.serialize_to_string(str);
		}
		str += "}";
		break;
	}
	case TYPE_STRING: {
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
		*this = game_logic::formula(str).execute();
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

std::string variant::string_cast() const
{
	switch(type_) {
	case TYPE_NULL:
		return "0";
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
		for(size_t i=0; i<list_->elements.size(); ++i) {
			const variant& var = list_->elements[i];
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
			res += "->";
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
					s << ", ";
				}
				first = false;
				s << input.name << " ";
				if(input.access == game_logic::FORMULA_READ_WRITE) {
					s << "(read-write) ";
				} else if(input.access == game_logic::FORMULA_WRITE_ONLY) {
					s << "(writeonly) ";
				}

				s << "-> " << callable_->query_value(input.name).to_debug_string(seen);
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
			s << "->";
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

	return s.str();
}

UNIT_TEST(variant_decimal)
{
	variant d(9876000, variant::DECIMAL_VARIANT);
	variant d2(4000, variant::DECIMAL_VARIANT);
	CHECK_EQ(d.as_decimal().value(), 9876000);
	CHECK_EQ(d.as_int(), 9);
	CHECK_EQ(d.string_cast(), "9.876000");
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
