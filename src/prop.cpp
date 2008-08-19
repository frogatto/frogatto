#include <iostream>
#include <map>

#include "formatter.hpp"
#include "prop.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
std::map<std::string, prop_ptr> props_map;
}

std::vector<const_prop_ptr> prop::all_props()
{
	std::vector<const_prop_ptr> res;
	for(std::map<std::string, prop_ptr>::const_iterator i = props_map.begin();
	    i != props_map.end(); ++i) {
		res.push_back(i->second);
	}

	return res;
}

void prop::init(wml::const_node_ptr node)
{
	std::cerr << "initing props...\n";
	wml::node::const_child_iterator i1 = node->begin_child("prop");
	wml::node::const_child_iterator i2 = node->end_child("prop");
	while(i1 != i2) {
		prop_ptr p(new prop(i1->second));
		props_map[p->id()] = p;
		std::cerr << "init prop: '" << p->id() << "'\n";
		++i1;
	}
}

const_prop_ptr prop::get(const std::string& id)
{
	std::map<std::string, prop_ptr>::const_iterator i = props_map.find(id);
	if(i != props_map.end()) {
		return i->second;
	} else {
		return const_prop_ptr();
	}
}

prop::prop(wml::const_node_ptr node)
  : id_(node->attr("id")), zorder_(wml::get_int(node, "zorder")), frame_(node)
{
}

prop_object::prop_object(int x, int y, const std::string& id)
  : type_(prop::get(id))
{
	if(!type_) {
		std::cerr << "could not find prop '" << id << "'\n";
		throw prop::error();
	}

	area_ = rect(x, y, type_->get_frame().width(), type_->get_frame().height());
}

prop_object::prop_object(wml::const_node_ptr node)
  : type_(prop::get(node->attr("id")))
{
	if(!type_) {
		std::cerr << "could not find prop '" << node->attr("id") << "'\n";
		throw prop::error();
	}

	area_ = rect(wml::get_int(node, "x"), wml::get_int(node, "y"),
	             type_->get_frame().width(), type_->get_frame().height());
}

void prop_object::draw() const
{
	type_->get_frame().draw(area_.x(), area_.y(), true);
}

bool prop_object::operator<(const prop_object& o) const
{
	return type_->zorder() < o.type_->zorder();
}

wml::node_ptr prop_object::write() const
{
	wml::node_ptr node(new wml::node("prop"));
	node->set_attr("id", type_->id());
	node->set_attr("x", formatter() << area_.x());
	node->set_attr("y", formatter() << area_.y());
	return node;
}

bool operator<(int zorder, const prop_object& o)
{
	return zorder < o.zorder();
}

bool operator<(const prop_object& o, int zorder)
{
	return o.zorder() < zorder;
}
