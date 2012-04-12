#ifndef FORMULA_CALLABLE_UTILS_HPP_INCLUDED
#define FORMULA_CALLABLE_UTILS_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include <vector>

#include "asserts.hpp"
#include "formula_callable.hpp"
#include "variant.hpp"

namespace game_logic
{

class slot_formula_callable : public formula_callable
{
public:
	slot_formula_callable() : value_names_(NULL), base_slot_(0)
	{}

	void set_names(const std::vector<std::string>* names) {
		value_names_ = names;
	}
	void set_fallback(const const_formula_callable_ptr& fallback) { fallback_ = fallback; }
	void add(const variant& val) { values_.push_back(val); }
	void reserve(size_t n) { values_.reserve(n); }

	variant get_value(const std::string& key) const {
		if(value_names_) {
			for(int n = 0; n != value_names_->size(); ++n) {
				if((*value_names_)[n] == key) {
					return values_[n];
				}
			}
		}

		if(fallback_) {
			return fallback_->query_value(key);
		}
		ASSERT_LOG(false, "GET VALUE " << key << " FROM SLOT CALLABLE");
		return variant();
	}

	variant get_value_by_slot(int slot) const {
		if(slot < base_slot_) {
			return fallback_->query_value_by_slot(slot);
		}

		slot -= base_slot_;
		ASSERT_INDEX_INTO_VECTOR(slot, values_);
		return values_[slot];
	}

	void clear() {
		value_names_ = 0;
		values_.clear();
		fallback_ = const_formula_callable_ptr();
	}

	void set_base_slot(int base) { base_slot_ = base; }

private:
	const std::vector<std::string>* value_names_;
	std::vector<variant> values_;
	const_formula_callable_ptr fallback_;

	int base_slot_;
};

}

#endif
