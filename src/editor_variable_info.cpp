#include <iostream>

#include "editor_variable_info.hpp"
#include "foreach.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

editor_variable_info::editor_variable_info(wml::const_node_ptr node)
  : name_(node->attr("name")), type_(TYPE_INTEGER)
{
	const std::string& type = node->attr("type");
	if(type == "x") {
		type_ = XPOSITION;
		std::cerr << "XPOS VARIABLE\n";
	} else if(type == "y") {
		type_ = YPOSITION;
	}
}

wml::node_ptr editor_variable_info::write() const
{
	wml::node_ptr node(new wml::node("var"));
	node->set_attr("name", name_);
	switch(type_) {
	case XPOSITION:
		node->set_attr("type", "x");
		break;
	case YPOSITION:
		node->set_attr("type", "y");
		break;
	}
	return node;
}

editor_entity_info::editor_entity_info(wml::const_node_ptr node)
{
	FOREACH_WML_CHILD(var_node, node, "var") {
		std::cerr << "CREATE VAR INFO...\n";	
		vars_.push_back(editor_variable_info(var_node));
	}
}

wml::node_ptr editor_entity_info::write() const
{
	wml::node_ptr node(new wml::node("editor_info"));
	foreach(const editor_variable_info& v, vars_) {
		node->add_child(v.write());
	}

	return node;
}
