#ifndef FORMULA_VARIABLE_STORAGE_HPP_INCLUDED
#define FORMULA_VARIABLE_STORAGE_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include "formula_callable.hpp"
#include "wml_node.hpp"

namespace game_logic
{

class formula_variable_storage : public formula_callable
{
public:
	formula_variable_storage();
	explicit formula_variable_storage(const std::map<std::string, variant>& m);

	bool equal_to(const std::map<std::string, variant>& m) const;

	void read(wml::const_node_ptr node);
	void write(wml::node_ptr node) const;
	void add(const std::string& key, const variant& value);
	void add(const formula_variable_storage& value);

	std::vector<variant>& values() { return values_; }
	const std::vector<variant>& values() const { return values_; }

private:
	variant get_value(const std::string& key) const;
	variant get_value_by_slot(int slot) const;
	void set_value(const std::string& key, const variant& value);
	void set_value_by_slot(int slot, const variant& value);

	void get_inputs(std::vector<formula_input>* inputs) const;
	
	std::vector<variant> values_;
	std::map<std::string, int> strings_to_values_;
};

typedef boost::intrusive_ptr<formula_variable_storage> formula_variable_storage_ptr;
typedef boost::intrusive_ptr<const formula_variable_storage> const_formula_variable_storage_ptr;

}

#endif
