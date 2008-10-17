#include <assert.h>
#include <iostream>
#include <map>

#include "formatter.hpp"
#include "wml_node.hpp"
#include "wml_schema.hpp"

namespace wml {

namespace {
std::map<std::string, schema> schemas;
}

void schema::init(wml::const_node_ptr node)
{
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
schema::attribute_info parse_attribute_info(const std::string& str)
{
	schema::attribute_info info;
	static const boost::regex pattern("(required|optional) (.*)");
	boost::smatch match;
	if(boost::regex_match(str, match, pattern)) {
		info.optional = (std::string(match[1].first, match[1].second) == "optional");
		std::string type(match[2].first, match[2].second);
		if(type == "integer") {
			info.type = schema::ATTR_INT;
		} else if(type == "boolean") {
			info.type = schema::ATTR_BOOL;
		} else if(type == "string") {
			info.type = schema::ATTR_STRING;
		} else if(type == "formula") {
			info.type = schema::ATTR_FORMULA;
		} else {
			static const boost::regex pattern("re (.*)");
			boost::smatch match;
			if(!boost::regex_match(type, match, pattern)) {
				std::cerr << "ILLEGAL SCHEMA ATTR: '" << str << "'\n";
				assert(false);
			}

			info.type = schema::ATTR_REGEX;
			info.re.reset(new boost::regex(std::string(match[1].first, match[1].second)));
		}
	} else {
		std::cerr << "ILLEGAL SCHEMA ATTR: '" << str << "'\n";
		assert(false);
	}

	return info;
}
}

schema::schema(wml::const_node_ptr node)
{
	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		assert(i->first.empty() == false);

		if(i->first[0] == '_') {
			std::string element(i->first.begin() + 1, i->first.end());
			static const boost::regex pattern("(optional|required|repeated) ([a-z_]+)");
			boost::smatch match;
			if(!boost::regex_match(i->second, match, pattern)) {
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
			std::cerr << "ADD ELEMENT: '" << i->first << "'\n";
		} else {
			attributes_[i->first] = parse_attribute_info(i->second);
		}
	}
}

void schema::validate_attribute(const std::string& name, const std::string& value) const
{
	attribute_map::const_iterator itor = attributes_.find(name);
	if(itor == attributes_.end()) {
		static const std::string DefaultStr = "default";
		itor = attributes_.find(DefaultStr);
	}

	if(itor == attributes_.end()) {
		generate_error(formatter() << "Unknown attribute: " << name);
	}

	switch(itor->second.type) {
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
			generate_error(formatter() << "Value for attribute " << name << " is " << value << " which is not an integer");
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
		if(itor->second.re) {
			boost::smatch match;
			if(!boost::regex_match(value, match, *itor->second.re)) {
				generate_error(formatter() << "Value for attribute " << name << " is " << value << " which does not match the required pattern");
			}
		}

		break;
	}

	default:
		assert(false);
	}
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

void schema::generate_error(const std::string& msg)
{
	std::cerr << "ERROR: " << msg << "\n";
	throw schema_error(msg);
}

}

#ifdef UNIT_TEST_WML_SCHEMA

int main(int argc, char** argv)
{
}

#endif
