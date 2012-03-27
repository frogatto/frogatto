#include <set>
#include <stack>
#include <string>

#include <stdio.h>

#include "asserts.hpp"
#include "foreach.hpp"
#include "variant_utils.hpp"
#include "wml_formula_callable.hpp"

namespace game_logic
{

namespace {
struct scope_info {
	std::set<const_wml_serializable_formula_callable_ptr> objects_to_write, objects_written;
};

std::stack<scope_info, std::vector<scope_info> > scopes;

}

void wml_formula_callable_serialization_scope::register_serialized_object(const_wml_serializable_formula_callable_ptr ptr, variant& node)
{
	ASSERT_LOG(scopes.empty() == false, "register_serialized_object() called when there is no wml_formula_callable_serialization_scope");
	scopes.top().objects_written.insert(ptr);

	char addr_buf[256];
	sprintf(addr_buf, "%p", ptr.get());
	node = node.add_attr(variant("_addr"), variant(addr_buf));
}

std::string  wml_formula_callable_serialization_scope::require_serialized_object(const_wml_serializable_formula_callable_ptr ptr)
{
	char addr_buf[256];
	sprintf(addr_buf, "%p", ptr.get());

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

variant wml_formula_callable_serialization_scope::write_objects() const
{
	variant_builder result;
	foreach(const_wml_serializable_formula_callable_ptr ptr, scopes.top().objects_to_write) {
		if(scopes.top().objects_written.count(ptr) != 0) {
			continue;
		}

		char addr_buf[256];
		sprintf(addr_buf, "%p", ptr.get());

		variant node(ptr->write_to_wml());
		node.add_attr(variant("_addr"), variant(addr_buf));
		result.add("character", node);
	}

	return result.build();
}

namespace {
std::map<intptr_t, wml_serializable_formula_callable_ptr> registered_objects;
}

void wml_formula_callable_read_scope::register_serialized_object(intptr_t addr, wml_serializable_formula_callable_ptr ptr)
{
	//fprintf(stderr, "REGISTER SERIALIZED: 0x%x\n", (int)addr);
	registered_objects[addr] = ptr;
}

wml_serializable_formula_callable_ptr wml_formula_callable_read_scope::get_serialized_object(intptr_t addr)
{
	return registered_objects[addr];
}

wml_formula_callable_read_scope::wml_formula_callable_read_scope()
{
}

wml_formula_callable_read_scope::~wml_formula_callable_read_scope()
{
	std::set<variant*> v;
	swap_variants_loading(v);
	for(std::set<variant*>::iterator i = v.begin(); i != v.end(); ++i) {
		variant& var = **i;
		//fprintf(stderr, "LOAD SERIALIZED: 0x%x\n", (int)var.as_callable_loading());
		var = variant(registered_objects[var.as_callable_loading()].get());
	}

	registered_objects.clear();
}

}
