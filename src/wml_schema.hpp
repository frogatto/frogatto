#ifndef WML_SCHEMA_HPP_INCLUDED
#define WML_SCHEMA_HPP_INCLUDED

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

#include "variant.hpp"
#include "wml_node_fwd.hpp"

namespace wml {

struct schema_error {
	explicit schema_error(const std::string& msg) : message(msg)
	{}
	std::string message;
};

class schema {
public:
	static void init(wml::const_node_ptr node);
	static const schema* get(const std::string& id);
	explicit schema(wml::const_node_ptr node);

	const std::string& id() const { return id_; }

	bool has_attribute(const std::string& name) const;
	void validate_attribute(const std::string& name, const std::string& value) const;
	const schema* validate_element(const std::string& name) const;
	void validate_node(wml::const_node_ptr node) const;

	variant attribute_to_variant(const std::string& name, const std::string& value) const;
	bool is_element_repeated(const std::string& name) const;

	enum ATTRIBUTE_TYPE { ATTR_INT, ATTR_BOOL, ATTR_STRING, ATTR_LIST, ATTR_FORMULA, ATTR_REGEX };
	struct attribute_info {
		bool optional;
		ATTRIBUTE_TYPE type;
		boost::shared_ptr<boost::regex> re;
		boost::shared_ptr<attribute_info> elements;
		int list_size;
		void validate(const std::string& name, const std::string& value) const;
	};

	enum ELEMENT_CARDINALITY { ELEMENT_OPTIONAL, ELEMENT_REQUIRED, ELEMENT_REPEATED };
	struct element_info {
		std::string type;
		ELEMENT_CARDINALITY cardinality;
	};

private:
	static void generate_error(const std::string& msg);
	std::string id_;

	typedef std::map<std::string, attribute_info> attribute_map;
	attribute_map attributes_;

	typedef std::map<std::string, element_info> element_map;
	element_map elements_;
};

}

#endif
