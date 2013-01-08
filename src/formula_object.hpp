#ifndef FORMULA_OBJECT_HPP_INCLUDED
#define FORMULA_OBJECT_HPP_INCLUDED

#include <set>
#include <string>
#include <vector>

#include <boost/function.hpp>

#include "formula.hpp"
#include "variant.hpp"
#include "wml_formula_callable.hpp"

namespace game_logic
{

class formula_class;

class formula_object : public game_logic::wml_serializable_formula_callable
{
public:
	static void visit_variants(variant v, boost::function<void (variant)> fn, std::vector<formula_object*>* seen=NULL);

	static void reload_classes();

	static boost::intrusive_ptr<formula_object> create(const std::string& type, variant args=variant());


	//construct with data representing private/internal represenation.
	explicit formula_object(variant data);
	virtual ~formula_object();
private:
	//construct with type and constructor parameters.
	//Don't call directly, use create() instead.
	explicit formula_object(const std::string& type, variant args=variant());
	void call_constructors(variant args);

	variant serialize_to_wml() const;

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	void get_inputs(std::vector<formula_input>* inputs) const;

	//overrides of the class's read-only properties.
	std::map<std::string, formula_ptr> property_overrides_;

	boost::intrusive_ptr<const formula_class> class_;
	variant private_data_;

	variant tmp_value_;

	//if this is non-zero, then private_data_ will be exposed via get_value.
	mutable int expose_private_data_;
};
}

#endif
