#include <iostream>

#include "wml_formula_adapter.hpp"
#include "wml_node.hpp"
#include "wml_schema.hpp"
#include "wml_writer.hpp"

namespace wml {
variant node_elements_callable::get_value(const std::string& key) const
{
	if(key == "_all") {
		std::vector<variant> items;
		wml::node::const_all_child_iterator i1 = node_->begin_children();
		wml::node::const_all_child_iterator i2 = node_->end_children();
		for(; i1 != i2; ++i1) {
			items.push_back(variant(new node_callable(*i1)));
		}

		return variant(&items);
	}

	const bool repeated = node_->get_schema() == NULL || node_->get_schema()->is_element_repeated(key);
	if(repeated) {
		std::vector<variant> items;
		wml::node::const_child_iterator i1 = node_->begin_child(key);
		wml::node::const_child_iterator i2 = node_->end_child(key);
		while(i1 != i2) {
			items.push_back(variant(new node_callable(i1->second)));
			++i1;
		}

		return variant(&items);
	} else {
		wml::const_node_ptr child = node_->get_child(key);
		if(child) {
			return variant(new node_callable(child));
		} else {
			return variant();
		}
	}
}

variant node_callable::get_value(const std::string& key) const
{
	const wml::schema* const schema = node_->get_schema();

	if(key == "_type") {
		if(schema) {
			return variant(schema->id());
		} else {
			return variant();
		}
	} else if(key == "elements") {
		if(elements_.is_null()) {
			elements_ = variant(new node_elements_callable(node_));
		}

		return elements_;
	}

	if(schema && schema->has_attribute(key) == false) {
		return node_callable_.query_value(key);
	}

	const std::string& value = node_->attr(key);
	if(schema) {
		return schema->attribute_to_variant(key, value);
	} else {
		return variant(value);
	}
}

void node_callable::set_value(const std::string& key, const variant& value)
{
	assert(mutable_node_);
	mutable_node_->set_attr(key, value.string_cast());
}

}
