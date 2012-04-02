#include <iostream>

#include "editor_variable_info.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

editor_variable_info::editor_variable_info(variant node)
  : name_(node["name"].as_string()), type_(TYPE_INTEGER), info_(node["info"].as_string_default()),
    help_(node["help"].as_string_default()),
    formula_(game_logic::formula::create_optional_formula(node["value"]))
{
	const std::string& type = node["type"].as_string_default();
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
		enum_values_ = util::split(node["enum_values"].as_string());
		ASSERT_LOG(enum_values_.empty() == false, "IN PROPERTY: " << name_ << " ENUM WITH NO VALUES SPECIFIED");
	} else if(type == "points") {
		type_ = TYPE_POINTS;
	}
}

variant editor_variable_info::write() const
{
	variant_builder node;
	node.add("name", name_);
	if(info_.empty() == false) {
		node.add("info", info_);
	}

	switch(type_) {
	case XPOSITION:
		node.add("type", "x");
		break;
	case YPOSITION:
		node.add("type", "y");
		break;
	case TYPE_LEVEL:
		node.add("type", "level");
		break;
	case TYPE_LABEL:
		node.add("type", "label");
		break;
	case TYPE_TEXT:
		node.add("type", "text");
		break;
	case TYPE_BOOLEAN:
		node.add("type", "boolean");
		break;
	case TYPE_ENUM:
		node.add("type", "enum");
		node.add("values", util::join(enum_values_));
		break;
	case TYPE_POINTS:
		node.add("type", "points");
		break;
	}
	return node.build();
}

editor_entity_info::editor_entity_info(variant node)
  : category_(node["category"].as_string()), help_(node["help"].as_string_default())
{
	foreach(variant var_node, node["var"].as_list()) {
		//std::cerr << "CREATE VAR INFO...\n";	
		vars_.push_back(editor_variable_info(var_node));
	}
}

variant editor_entity_info::write() const
{
	variant_builder node;
	node.add("category", category_);
	foreach(const editor_variable_info& v, vars_) {
		node.add("var", v.write());
	}

	return node.build();
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
