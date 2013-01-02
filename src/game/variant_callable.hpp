#ifndef VARIANT_CALLABLE_HPP_INCLUDED
#define VARIANT_CALLABLE_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include "formula_callable.hpp"
#include "variant.hpp"

class variant_callable;

typedef boost::intrusive_ptr<variant_callable> variant_callable_ptr;
typedef boost::intrusive_ptr<const variant_callable> const_variant_callable_ptr;

class variant_callable : public game_logic::formula_callable
{
public:
	static variant create(variant* v);

	const variant& get_value() const { return value_; }
private:
	variant_callable(const variant& v);

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	variant create_for_list(const variant& list) const;

	variant value_;
};

#endif
