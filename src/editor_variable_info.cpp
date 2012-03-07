#include <iostream>

#include "editor_variable_info.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

editor_variable_info::editor_variable_info(wml::const_node_ptr node)
  : name_(node->attr("name")), type_(TYPE_INTEGER), info_(node->attr("info")),
    help_(node->attr("help")),
    formula_(game_logic::formula::create_optional_formula(node->attr("value")))
{
	const std::string& type = node->attr("type");
	if(type == "x") {
		type_ = XPOSITION;
		//std::cerr << "XPOS VARIABLE\n";
	} else if(type == "y") {
		type_ = YPOSITION;
	} else if(type == "level") {
		type_ = TYPE_LEVEL;
	} else if(type == "label") {
		type_ = TYPE_LABEL;
	} else if(type == "text") {
		type_ = TYPE_TEXT;
	} else if(type == "boolean") {
		type_ = TYPE_BOOLEAN;
	} else if(type == "enum") {
		type_ = TYPE_ENUM;
		enum_values_ = util::split(node->attr("enum_values"));
		ASSERT_LOG(enum_values_.empty() == false, "IN PROPERTY: " << name_ << " ENUM WITH NO VALUES SPECIFIED");
	}
}

wml::node_ptr editor_variable_info::write() const
{
	wml::node_ptr node(new wml::node("var"));
	node->set_attr("name", name_);
	if(info_.empty() == false) {
		node->set_attr("info", info_);
	}

	switch(type_) {
	case XPOSITION:
		node->set_attr("type", "x");
		break;
	case YPOSITION:
		node->set_attr("type", "y");
		break;
	case TYPE_LEVEL:
		node->set_attr("type", "level");
		break;
	case TYPE_LABEL:
		node->set_attr("type", "label");
		break;
	case TYPE_TEXT:
		node->set_attr("type", "text");
		break;
	case TYPE_BOOLEAN:
		node->set_attr("type", "boolean");
		break;
	case TYPE_ENUM:
		node->set_attr("type", "enum");
		node->set_attr("values", util::join(enum_values_));
		break;
	}
	return node;
}

editor_entity_info::editor_entity_info(wml::const_node_ptr node)
  : category_(node->attr("category")), help_(node->attr("help"))
{
	FOREACH_WML_CHILD(var_node, node, "var") {
		//std::cerr << "CREATE VAR INFO...\n";	
		vars_.push_back(editor_variable_info(var_node));
	}
}

wml::node_ptr editor_entity_info::write() const
{
	wml::node_ptr node(new wml::node("editor_info"));
	node->set_attr("category", category_);
	foreach(const editor_variable_info& v, vars_) {
		node->add_child(v.write());
	}

	return node;
}

const editor_variable_info* editor_entity_info::get_var_info(const std::string& var_name) const
{
	foreach(const editor_variable_info& v, vars_) {
		if(v.variable_name() == var_name) {
			return &v;
		}
	}

	return NULL;
}
