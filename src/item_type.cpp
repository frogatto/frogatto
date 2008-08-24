#include <assert.h>
#include <iostream>
#include <map>

#include "item_type.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
typedef std::map<std::string, const_item_type_ptr> item_map;
item_map cache;
}

void item_type::init(wml::const_node_ptr node)
{
	wml::node::const_child_iterator i1 = node->begin_child("item");
	wml::node::const_child_iterator i2 = node->end_child("item");
	for(; i1 != i2; ++i1) {
		cache[i1->second->attr("id")].reset(new item_type(i1->second));
	}
}

const_item_type_ptr item_type::get(const std::string& id)
{
	assert(cache[id]);
	return cache[id];
}

item_type::item_type(wml::const_node_ptr node)
  : id_(node->attr("id")),
    frame_(node->get_child("animation")),
	destroy_on_touch_(wml::get_bool(node, "destroy_on_touch", false)),
	automatic_touch_(wml::get_bool(node, "automatic_touch", true)),
	on_touch_music_(node->attr("on_touch_music")),
	touch_condition_(game_logic::formula::create_optional_formula(node->attr("touch_condition"))),
	target_(game_logic::formula::create_optional_formula(node->attr("target"))),
	on_touch_particles_(node->attr("on_touch_particles")),
	num_on_touch_particles_(wml::get_int(node, "num_particles"))
{
	if(node->get_child("touched")) {
		touched_frame_.reset(new frame(node->get_child("touched")));
	}

	wml::const_node_ptr ontouch = node->get_child("ontouch");
	if(ontouch) {
		for(wml::node::const_attr_iterator i = ontouch->begin_attr();
		    i != ontouch->end_attr(); ++i) {
			on_touch_[i->first].reset(new game_logic::formula(i->second));
		}
	}
}

const frame* item_type::touched_frame() const
{
	return touched_frame_.get();
}
