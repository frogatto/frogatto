#ifndef FORMULA_OBJECT_HPP_INCLUDED
#define FORMULA_OBJECT_HPP_INCLUDED

#include <string>

#include "variant.hpp"
#include "wml_formula_callable.hpp"

namespace game_logic
{

class formula_class;

class formula_object : public game_logic::wml_serializable_formula_callable
{
public:
	static void reload_classes();

	//construct with type and constructor parameters.
	explicit formula_object(const std::string& type, variant args=variant());

	//construct with data representing private/internal represenation.
	explicit formula_object(variant data);
	virtual ~formula_object();
private:

	variant serialize_to_wml() const;

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	void get_inputs(std::vector<formula_input>* inputs) const;

	boost::intrusive_ptr<const formula_class> class_;
	variant private_data_;

	variant tmp_value_;

	//if this is non-zero, then private_data_ will be exposed via get_value.
	mutable int expose_private_data_;
};
}

#endif
