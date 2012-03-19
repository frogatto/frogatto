#include <assert.h>
#include <iostream>
#include <map>

#include "foreach.hpp"
#include "formatter.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_schema.hpp"

#include "asserts.hpp"

namespace wml {

namespace {
std::map<std::string, schema> schemas;
std::map<std::string, std::string> data_types;
}

void schema::init(wml::const_node_ptr node)
{
	for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
		data_types[i->first] = i->second;
	}

	wml::node::const_all_child_iterator i1 = node->begin_children();
	wml::node::const_all_child_iterator i2 = node->end_children();
	while(i1 != i2) {
		schemas.insert(std::pair<std::string, schema>((*i1)->name(), schema(*i1)));
		++i1;
	}
}

const schema* schema::get(const std::string& id)
{
	std::map<std::string, schema>::const_iterator itor = schemas.find(id);
	if(itor != schemas.end()) {
		return &itor->second;
	}

	return NULL;
}

namespace {

void parse_type_info(const std::string& type, schema::attribute_info& info, int depth=0)
{
	if(depth == 10) {
		std::cerr << "SCHEMA ERROR: self-referent types recurse too deeply in schema!\n";
		return;
	}

	static const boost::regex re_pattern("re +(.*)");
	static const boost::regex list_sized_pattern("list *\\[([0-9]+)\\] *(.*)");
	static const boost::regex list_pattern("list +(.*)");
	boost::smatch match;
	info.list_size = -1;
	if(type == "integer") {
		info.type = schema::ATTR_INT;
	} else if(type == "boolean") {
		info.type = schema::ATTR_BOOL;
	} else if(type == "string") {
		info.type = schema::ATTR_STRING;
	} else if(type == "formula") {
		info.type = schema::ATTR_FORMULA;
	} else if(boost::regex_match(type, match, re_pattern)) {
		info.type = schema::ATTR_REGEX;
		info.re.reset(new boost::regex(std::string(match[1].first, match[1].second)));
	} else if(boost::regex_match(type, match, list_sized_pattern)) {
		info.type = schema::ATTR_LIST;
		info.list_size = atoi(std::string(match[1].first, match[1].second).c_str());
		info.elements.reset(new schema::attribute_info);
		parse_type_info(std::string(match[2].first, match[2].second), *info.elements);
	} else if(boost::regex_match(type, match, list_pattern)) {
		info.type = schema::ATTR_LIST;
		info.elements.reset(new schema::attribute_info);
		parse_type_info(std::string(match[1].first, match[1].second), *info.elements);
	} else {
		std::map<std::string, std::string>::const_iterator itor = data_types.find(type);
		if(itor != data_types.end()) {
			parse_type_info(itor->second, info, depth+1);
		} else {
			std::cerr << "ILLEGAL SCHEMA TYPE: '" << type << "'\n";
		}
	}
}

schema::attribute_info parse_attribute_info(const std::string& str)
{
	schema::attribute_info info;
    static const boost::regex pattern("(required|optional) +(.*)");
	boost::smatch match;
	if(boost::regex_match(str, match, pattern)) {
		info.optional = (std::string(match[1].first, match[1].second) == "optional");
		std::string type(match[2].first, match[2].second);
		parse_type_info(type, info);
	} else {
		std::cerr << "ILLEGAL SCHEMA ATTR: '" << str << "'\n";
		assert(false);
	}
	return info;
}
}

schema::schema(wml::const_node_ptr node)
  : id_(node->name()), default_prefix_(node->attr("default_prefix"))
{
	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		assert(i->first.empty() == false);

		if(i->first[0] == '_') {
			std::string element(i->first.begin() + 1, i->first.end());
			static const boost::regex pattern("(optional|required|repeated) ([a-z_]+)");
			boost::smatch match;
			if(!boost::regex_match(i->second.str(), match, pattern)) {
				std::cerr << "ILLEGAL SCHEMA ELEMENT: '" << i->second << "'\n";
				assert(false);
			}

			element_info info;
			info.type = std::string(match[2].first, match[2].second);

			std::string cardinality(match[1].first, match[1].second);
			if(cardinality == "optional") {
				info.cardinality = ELEMENT_OPTIONAL;
			} else if(cardinality == "required") {
				info.cardinality = ELEMENT_REQUIRED;
			} else {
				info.cardinality = ELEMENT_REPEATED;
			}

			elements_[element] = info;
		} else {
			if(i->first != "default_prefix") {
				attributes_[i->first] = parse_attribute_info(i->second);
			}
		}
	}
}

void schema::attribute_info::validate(const std::string& name, const std::string& value) const
{
	switch(type) {
	case ATTR_INT: {
		static const boost::regex pattern("-?[0-9]+");
		boost::smatch match;
		if(!boost::regex_match(value, match, pattern)) {
			generate_error(formatter() << "Value for attribute " << name << " is " << value << " which is not an integer");
		}
		break;
	}

	case ATTR_BOOL: {
		static const boost::regex pattern("(true|false|yes|no)");
		boost::smatch match;
		if(!boost::regex_match(value, match, pattern)) {
			generate_error(formatter() << "Value for attribute " << name << " is " << value << " which is not a boolean.");
		}
		break;
	}

	case ATTR_STRING: {
		break;
	}

	case ATTR_FORMULA: {
		// TODO: validate it's a valid formula.
		break;
	}
	
	case ATTR_REGEX: {
		if(re) {
			boost::smatch match;
			if(!boost::regex_match(value, match, *re)) {
				generate_error(formatter() << "Value for attribute " << name << " is " << value << " which is not in the correct format");
			}
		}

		break;
	}

	case ATTR_LIST: {
		if(elements) {
			std::vector<std::string> items = util::split(value);
			if(list_size != -1 && list_size != items.size()) {
				generate_error(formatter() << "Expected " << list_size << " items in list, but found " << items.size());
			}
			foreach(const std::string& item, items) {
				elements->validate(name, item);
			}
		}
		break;
	}

	default:
		assert(false);
	}
}

bool schema::has_attribute(const std::string& name) const
{
	return attributes_.count(name) != 0;
}

void schema::validate_attribute(const std::string& name, const std::string& value) const
{
	attribute_map::const_iterator itor = attributes_.find(name);
	if(itor == attributes_.end() && name.size() <= default_prefix_.size() &&
	 std::equal(default_prefix_.begin(), default_prefix_.end(), name.begin())) {
		static const std::string DefaultStr = "default";
		itor = attributes_.find(DefaultStr);
	}

	if(itor == attributes_.end()) {
		generate_error(formatter() << "Unknown attribute: " << name);
	}

	itor->second.validate(name, value);
}

const schema* schema::validate_element(const std::string& name) const
{
	element_map::const_iterator itor = elements_.find(name);
	if(itor == elements_.end()) {
		generate_error(formatter() << "Unrecognized element: " << name);
	}

	return get(itor->second.type);
}

void schema::validate_node(wml::const_node_ptr node) const
{
	for(attribute_map::const_iterator i = attributes_.begin(); i != attributes_.end(); ++i) {
		if(i->second.optional == false && node->has_attr(i->first) == false) {
			generate_error(formatter() << "Required attribute " << i->first << " not found");
		}
	}

	for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
		validate_attribute(i->first, i->second);
	}

	for(element_map::const_iterator i = elements_.begin(); i != elements_.end(); ++i) {
		switch(i->second.cardinality) {
		case ELEMENT_OPTIONAL: {
			wml::node::const_child_iterator i1 = node->begin_child(i->first);
			wml::node::const_child_iterator i2 = node->end_child(i->first);
			if(i1 != i2) {
				++i1;
			}

			if(i1 != i2) {
				generate_error(formatter() << "Element " << i->first << " appears too many times; it should appear at most once.");
			}
			break;
		}
		case ELEMENT_REQUIRED:
			if(!node->get_child(i->first)) {
				generate_error(formatter() << "Required element " << i->first << " not found");
			}
			break;
		case ELEMENT_REPEATED:
			break;
		default:
			assert(false);
		}
	}
}

namespace {
variant convert_attribute_to_variant(const schema::attribute_info& info, const std::string& value) {
	switch(info.type) {
	case schema::ATTR_INT:
		return variant(atoi(value.c_str()));
	case schema::ATTR_BOOL:
		return variant((value == "yes" || value == "true") ? 1 : 0);
	case schema::ATTR_LIST: {
		std::vector<variant> v;
		std::vector<std::string> items = util::split(value);
		foreach(const std::string& item, items) {
			v.push_back(convert_attribute_to_variant(*info.elements, item));
		}

		return variant(&v);
	}
	case schema::ATTR_STRING:
	case schema::ATTR_FORMULA:
	case schema::ATTR_REGEX:
		return variant(value);
	default:
		assert(false);
	}
}
}

variant schema::attribute_to_variant(const std::string& name, const std::string& value) const
{
	attribute_map::const_iterator itor = attributes_.find(name);
	if(itor == attributes_.end()) {
		return variant(value);
	}

	return convert_attribute_to_variant(itor->second, value);
}

bool schema::is_element_repeated(const std::string& name) const
{
	element_map::const_iterator itor = elements_.find(name);
	if(itor != elements_.end()) {
		return itor->second.cardinality == ELEMENT_REPEATED;
	} else {
		return true;
	}
}

void schema::generate_error(const std::string& msg)
{
	throw schema_error(msg);
}

}

#ifdef UNIT_TEST_WML_SCHEMA

int main(int argc, char** argv)
{
}

#endif
