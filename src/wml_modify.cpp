#include <iostream>

#include "foreach.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"
#include "wml_modify.hpp"

namespace wml {
modifier::modifier(variant node)
{
	foreach(variant v, node["children"].as_list()) {
		modification mod;
		mod.target.reset(new game_logic::formula(v["_target"]));
		mod.add_children = util::split(v["add"].as_string());
		mod.add_if_not_present_children = util::split(v["add_if_not_present"].as_string());
		mod.remove_children = util::split(v["remove"].as_string());
		mods_.push_back(mod);
	}
/*
	i1 = node->begin_child("set");
	i2 = node->end_child("set");
	while(i1 != i2) {
		modification mod;
		mod.target.reset(new game_logic::formula(i1->second->attr("_target")));
		for(wml::node::const_attr_iterator i = i1->second->begin_attr();
		    i != i1->second->end_attr(); ++i) {
			if(i->first != "_target") {
				mod.attr[i->first].reset(new game_logic::formula(i->second));
			}
		}

		mods_.push_back(mod);
		++i1;
	}

	i1 = node->begin_child("set_str");
	i2 = node->end_child("set_str");
	while(i1 != i2) {
		modification mod;
		mod.target.reset(new game_logic::formula(i1->second->attr("_target")));
		for(wml::node::const_attr_iterator i = i1->second->begin_attr();
		    i != i1->second->end_attr(); ++i) {
			if(i->first != "_target") {
				mod.attr[i->first] = game_logic::formula::create_string_formula(i->second);
			}
		}

		mods_.push_back(mod);
		++i1;
	}
*/
}

void modifier::modify_target(variant target, const modification& mod) {
/*
	const std::map<std::string, game_logic::const_formula_ptr>& mods = mod.attr;
	if(target.is_list()) {
		for(int n = 0; n != target.num_elements(); ++n) {
			modify_target(target[n], mod);
		}

		return;
	}

	if(target.is_callable()) {
		wml::node_callable* c = target.try_convert<wml::node_callable>();
		if(c) {
			foreach(const std::string& add, mod.add_children) {
				c->raw_node()->add_child(wml::node_ptr(new wml::node(add)));
			}

			foreach(const std::string& add, mod.add_if_not_present_children) {
				if(!c->raw_node()->get_child(add)) {
					c->raw_node()->add_child(wml::node_ptr(new wml::node(add)));
				}
			}

			foreach(const std::string& remove, mod.remove_children) {
				while(c->raw_node()->get_child(remove)) {
					c->raw_node()->erase_child(c->raw_node()->get_child(remove));
				}
			}
		}

		for(std::map<std::string, game_logic::const_formula_ptr>::const_iterator i = mods.begin(); i != mods.end(); ++i) {
			variant value = i->second->execute(*target.as_callable());
			target.mutable_callable()->mutate_value(i->first, value);
		}
	}*/
}

variant modifier::modify(variant doc) const
{
	return doc;
		/*
	game_logic::formula_callable_ptr callable(new wml::node_callable(doc));
	foreach(const modification& mod, mods_) {
		variant target = mod.target->execute(*callable);
		modify_target(target, mod);
	}*/
}

} //namespace wml

#ifdef UNIT_TEST_WML_MODIFY

int main(int argc, char** argv) {
	
}

#endif
