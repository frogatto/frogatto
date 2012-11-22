#include <set>
#include <stack>
#include <string>

#include <boost/bind.hpp>

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

namespace {
void add_object_to_set(variant v, std::set<wml_serializable_formula_callable*>* set, std::set<std::string>* already_recorded) {
	if(v.is_map()) {
		variant addr = v["_addr"];
		if(addr.is_string()) {
			already_recorded->insert(addr.as_string());
		}

		return;
	}

	if(!v.is_callable()) {
		return;
	}

	wml_serializable_formula_callable* ptr = v.try_convert<wml_serializable_formula_callable>();
	if(ptr) {
		set->insert(ptr);
	}
}
}

variant wml_formula_callable_serialization_scope::write_objects(variant obj) const
{
	std::map<variant, variant> res;
	std::set<wml_serializable_formula_callable*> objects;
	std::set<std::string> already_known;
	visit_variants(obj, boost::bind(add_object_to_set, _1, &objects, &already_known));

	std::vector<variant> results_list;
	foreach(wml_serializable_formula_callable* item, objects) {
		char addr_buf[256];
		sprintf(addr_buf, "%p", item);
		std::string key(addr_buf);
		if(already_known.count(key)) {
			continue;
		}

		results_list.push_back(item->write_to_wml());
	}

	res[variant("character")] = variant(&results_list);

	return variant(&res);
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

	variant::resolve_delayed();

	registered_objects.clear();
}

bool wml_formula_callable_read_scope::try_load_object(intptr_t id, variant& v)
{
	std::map<intptr_t, wml_serializable_formula_callable_ptr>::const_iterator itor = registered_objects.find(id);
	if(itor != registered_objects.end()) {
		v = variant(itor->second.get());
		return true;
	} else {
		return false;
	}
}

}
