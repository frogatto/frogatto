#ifndef WML_FORMULA_ADAPTER_HPP_INCLUDED
#define WML_FORMULA_ADAPTER_HPP_INCLUDED

#include "formula_callable.hpp"
#include "variant.hpp"
#include "wml_node_fwd.hpp"

namespace wml {
class node_elements_callable : public game_logic::formula_callable
{
public:
	explicit node_elements_callable(const_node_ptr n) : node_(n)
	{}
private:
	variant get_value(const std::string& key) const;
	const_node_ptr node_;
};

class node_callable : public game_logic::formula_callable
{
public:
	explicit node_callable(node_ptr n) : mutable_node_(n.get()), node_(n), node_callable_(n)
	{}
	explicit node_callable(const_node_ptr n) : mutable_node_(const_cast<node*>(n.get())), node_(n), node_callable_(n)
	{}

	wml::node* raw_node() { return mutable_node_; }
private:
	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);
	node* mutable_node_;
	const_node_ptr node_;
	mutable variant elements_;
	node_elements_callable node_callable_;
};

}

#endif
