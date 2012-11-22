#ifndef VARIANT_HPP_INCLUDED
#define VARIANT_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <sstream>

#include <stdint.h>

#include <string.h>

#include "decimal.hpp"
#include "formula_fwd.hpp"

namespace game_logic {
class formula_callable;
class formula_expression;
}

void push_call_stack(const game_logic::formula_expression* frame);
void pop_call_stack();
std::string get_call_stack();
std::string get_full_call_stack();

const std::vector<const game_logic::formula_expression*>& get_expression_call_stack();

struct call_stack_manager {
	explicit call_stack_manager(const game_logic::formula_expression* str) {
		push_call_stack(str);
	}

	~call_stack_manager() {
		pop_call_stack();
	}
};

class variant;
void swap_variants_loading(std::set<variant*>& v);

struct variant_list;
struct variant_string;
struct variant_map;
struct variant_fn;
struct variant_delayed;

struct type_error {
	explicit type_error(const std::string& str);
	std::string message;
};

static const int64_t VARIANT_DECIMAL_PRECISION = DECIMAL_PRECISION;

class variant {
public:
	enum DECIMAL_VARIANT_TYPE { DECIMAL_VARIANT };

	static variant from_bool(bool b) { variant v; v.type_ = VARIANT_TYPE_BOOL; v.bool_value_ = b; return v; }

	static variant create_delayed(game_logic::const_formula_ptr f, boost::intrusive_ptr<const game_logic::formula_callable> callable);
	static void resolve_delayed();

	variant() : type_(VARIANT_TYPE_NULL), int_value_(0) {}
	explicit variant(int n) : type_(VARIANT_TYPE_INT), int_value_(n) {}
	explicit variant(unsigned int n) : type_(VARIANT_TYPE_INT), int_value_(n) {}
	explicit variant(long unsigned int n) : type_(VARIANT_TYPE_INT), int_value_(n) {}
	explicit variant(decimal d) : type_(VARIANT_TYPE_DECIMAL), decimal_value_(d.value()) {}
	explicit variant(double f) : type_(VARIANT_TYPE_DECIMAL), decimal_value_(decimal(f).value()) {}
	variant(int64_t n, DECIMAL_VARIANT_TYPE) : type_(VARIANT_TYPE_DECIMAL), decimal_value_(n) {}
	explicit variant(const game_logic::formula_callable* callable);
	explicit variant(std::vector<variant>* array);
	explicit variant(const char* str);
	explicit variant(const std::string& str);
	static variant create_translated_string(const std::string& str);
	static variant create_translated_string(const std::string& str, const std::string& translation);
	explicit variant(std::map<variant,variant>* map);
	variant(game_logic::const_formula_ptr, const std::vector<std::string>& args, const game_logic::formula_callable& callable, int base_slot, const std::vector<variant>& default_args);

	static variant create_variant_under_construction(intptr_t id);

	//only call the non-inlined release() function if we have a type
	//that needs releasing.
	~variant() { if(type_ > VARIANT_TYPE_INT) { release(); } }

	variant(const variant& v) {
		type_ = v.type_;
		value_ = v.value_;
		if(type_ > VARIANT_TYPE_INT) {
			increment_refcount();
		}
	}

	const variant& operator=(const variant& v);

	const variant& operator[](size_t n) const;
	const variant& operator[](const variant v) const;
	const variant& operator[](const std::string& key) const;
	size_t num_elements() const;

	variant get_list_slice(int begin, int end) const;

	bool has_key(const variant& key) const;
	bool has_key(const std::string& key) const;

	variant operator()(const std::vector<variant>& args) const;

	variant get_member(const std::string& str) const;

	//unsafe function which is called on an integer variant and returns
	//direct access to the underlying integer. Should only be used
	//when high performance is needed.
	int& int_addr() { must_be(VARIANT_TYPE_INT); return int_value_; }

	bool is_string() const { return type_ == VARIANT_TYPE_STRING; }
	bool is_null() const { return type_ == VARIANT_TYPE_NULL; }
	bool is_bool() const { return type_ == VARIANT_TYPE_BOOL; }
	bool is_numeric() const { return is_int() || is_decimal(); }
	bool is_int() const { return type_ == VARIANT_TYPE_INT; }
	bool is_decimal() const { return type_ == VARIANT_TYPE_DECIMAL; }
	bool is_map() const { return type_ == VARIANT_TYPE_MAP; }
	bool is_function() const { return type_ == VARIANT_TYPE_FUNCTION; }
	int as_int(int default_value=0) const { if(type_ == VARIANT_TYPE_NULL) { return default_value; } if(type_ == VARIANT_TYPE_DECIMAL) { return int( decimal_value_/VARIANT_DECIMAL_PRECISION ); } if(type_ == VARIANT_TYPE_BOOL) { return bool_value_ ? 1 : 0; } must_be(VARIANT_TYPE_INT); return int_value_; }
	decimal as_decimal(decimal default_value=decimal()) const { if(type_ == VARIANT_TYPE_NULL) { return default_value; } if(type_ == VARIANT_TYPE_INT) { return decimal::from_raw_value(int64_t(int_value_)*VARIANT_DECIMAL_PRECISION); } must_be(VARIANT_TYPE_DECIMAL); return decimal::from_raw_value(decimal_value_); }
	bool as_bool(bool default_value) const;
	bool as_bool() const;

	bool is_list() const { return type_ == VARIANT_TYPE_LIST; }

	std::vector<variant> as_list() const;
	const std::map<variant,variant>& as_map() const;

	std::vector<std::string> as_list_string() const;
	std::vector<std::string> as_list_string_optional() const;
	std::vector<int> as_list_int() const;
	std::vector<decimal> as_list_decimal() const;

	std::vector<int> as_list_int_optional() const { if(is_null()) return std::vector<int>(); else return as_list_int(); }
	std::vector<decimal> as_list_decimal_optional() const { if(is_null()) return std::vector<decimal>(); else return as_list_decimal(); }

	const std::string* filename() const { return 0; }
	int line_number() const { return -1; }

	//modifies the map to add an attribute. Note that if the map is referenced
	//by other variants, it will make a copy of it first.
	variant add_attr(variant key, variant value);
	variant remove_attr(variant key);

	//A dangerous function which mutates the object. Should only do this
	//in contexts where we're sure it's safe.
	void add_attr_mutation(variant key, variant value);
	void remove_attr_mutation(variant key);

	//functions which look up maps and lists and gets direct access by address
	//to the member values. These are dangerous functions which should be
	//used judiciously!
	variant* get_attr_mutable(variant key);
	variant* get_index_mutable(int index);

	//binds a closure to a lambda function.
	variant bind_closure(const game_logic::formula_callable* callable);

	std::string as_string_default(const char* default_value=NULL) const;
	const std::string& as_string() const;

	bool is_callable() const { return type_ == VARIANT_TYPE_CALLABLE; }
	const game_logic::formula_callable* as_callable() const {
		must_be(VARIANT_TYPE_CALLABLE); return callable_; }
	game_logic::formula_callable* mutable_callable() const {
		must_be(VARIANT_TYPE_CALLABLE); return mutable_callable_; }

	intptr_t as_callable_loading() const { return callable_loading_; }

	template<typename T>
	T* try_convert() const {
		if(!is_callable()) {
			return NULL;
		}

		return dynamic_cast<T*>(mutable_callable());
	}

	template<typename T>
	T* convert_to() const {
		T* res = dynamic_cast<T*>(mutable_callable());
		if(!res) {
			throw type_error("could not convert type");
		}

		return res;
	}

	variant operator+(const variant&) const;
	variant operator-(const variant&) const;
	variant operator*(const variant&) const;
	variant operator/(const variant&) const;
	variant operator^(const variant&) const;
	variant operator%(const variant&) const;
	variant operator-() const;

	bool operator==(const variant&) const;
	bool operator!=(const variant&) const;
	bool operator<(const variant&) const;
	bool operator>(const variant&) const;
	bool operator<=(const variant&) const;
	bool operator>=(const variant&) const;

	variant get_keys() const;
	variant get_values() const;

	void serialize_to_string(std::string& str) const;
	void serialize_from_string(const std::string& str);

	int refcount() const;
	void make_unique();

	std::string string_cast() const;

	std::string to_debug_string(std::vector<const game_logic::formula_callable*>* seen=NULL) const;

	std::string write_json(bool pretty=true) const;
	void write_json(std::ostream& s) const;

	void write_json_pretty(std::ostream& s, std::string indent) const;

	enum TYPE { VARIANT_TYPE_NULL, VARIANT_TYPE_BOOL, VARIANT_TYPE_INT, VARIANT_TYPE_DECIMAL, VARIANT_TYPE_CALLABLE, VARIANT_TYPE_CALLABLE_LOADING, VARIANT_TYPE_LIST, VARIANT_TYPE_STRING, VARIANT_TYPE_MAP, VARIANT_TYPE_FUNCTION, VARIANT_TYPE_DELAYED };
	TYPE type() const { return type_; }

	struct debug_info {
		debug_info() : filename(0), line(-1), column(-1), end_line(-1), end_column(-1)
		{}
		std::string message() const;
		const std::string* filename;
		int line, column;
		int end_line, end_column;
	};

	void set_debug_info(const debug_info& info);
	const debug_info* get_debug_info() const;

	std::pair<variant*,variant*> range() const;

private:
	void must_be(TYPE t) const {
#if !TARGET_OS_IPHONE
		if(type_ != t) {
			throw_type_error(t);
		}
#endif
	}

	void throw_type_error(TYPE expected) const;

	TYPE type_;
	union {
		bool bool_value_;
		int int_value_;
		int64_t decimal_value_;
		const game_logic::formula_callable* callable_;
		game_logic::formula_callable* mutable_callable_;
		intptr_t callable_loading_;
		variant_list* list_;
		variant_string* string_;
		variant_map* map_;
		variant_fn* fn_;
		variant_delayed* delayed_;
		debug_info* debug_info_;

		int64_t value_;
	};

	//function to initialize the variant as a list, returning the
	//underlying list vector to be initialized. If the variant is already a list,
	//and holds the only reference to that list, then that list object may
	//be cleared and re-used as a performance optimization.

	void increment_refcount();
	void release();
};

std::ostream& operator<<(std::ostream& os, const variant& v);

typedef std::pair<variant,variant> variant_pair;

template<typename T>
T* convert_variant(const variant& v) {
	T* res = dynamic_cast<T*>(v.mutable_callable());
	if(!res) {
		throw type_error("could not convert type");
	}

	return res;
}

template<typename T>
T* try_convert_variant(const variant& v) {
	if(!v.is_callable()) {
		return NULL;
	}

	return dynamic_cast<T*>(v.mutable_callable());
}

#endif
