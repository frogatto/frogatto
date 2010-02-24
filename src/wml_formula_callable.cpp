#include <set>
#include <stack>
#include <string>

#include <stdio.h>

#include "asserts.hpp"
#include "foreach.hpp"
#include "wml_formula_callable.hpp"
#include "wml_node.hpp"

namespace game_logic
{

namespace {
struct scope_info {
	std::set<const_wml_serializable_formula_callable_ptr> objects_to_write, objects_written;
};

std::stack<scope_info> scopes;

}

void wml_formula_callable_serialization_scope::register_serialized_object(const_wml_serializable_formula_callable_ptr ptr, wml::node_ptr node)
{
	ASSERT_LOG(scopes.empty() == false, "register_serialized_object() called when there is no wml_formula_callable_serialization_scope");
	scopes.top().objects_written.insert(ptr);

	char addr_buf[256];
	sprintf(addr_buf, "%p", ptr.get());
	node->set_attr("_addr", addr_buf);
}

std::string  wml_formula_callable_serialization_scope::require_serialized_object(const_wml_serializable_formula_callable_ptr ptr)
{
	char addr_buf[256];
	sprintf(addr_buf, "serialize_load(%p)", ptr.get());

	ASSERT_LOG(scopes.empty() == false, "require_serialized_object() called when there is no wml_formula_callable_serialization_scope");
	scopes.top().objects_to_write.insert(ptr);

	return addr_buf;
}

bool wml_formula_callable_serialization_scope::is_active()
{
	return scopes.empty() == false;
}

wml_formula_callable_serialization_scope::wml_formula_callable_serialization_scope()
{
	scopes.push(scope_info());
}

wml_formula_callable_serialization_scope::~wml_formula_callable_serialization_scope()
{
	scopes.pop();
}

wml::node_ptr wml_formula_callable_serialization_scope::write_objects() const
{
	wml::node_ptr result(new wml::node("serialized_objects"));
	foreach(const_wml_serializable_formula_callable_ptr ptr, scopes.top().objects_to_write) {
		if(scopes.top().objects_written.count(ptr) != 0) {
			continue;
		}

		char addr_buf[256];
		sprintf(addr_buf, "%p", ptr.get());

		wml::node_ptr node(ptr->write_to_wml());
		node->set_attr("_addr", addr_buf);
		result->add_child(node);
	}

	return result;
}

}
