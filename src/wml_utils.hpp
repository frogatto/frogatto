
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef WML_UTILS_HPP_INCLUDED
#define WML_UTILS_HPP_INCLUDED

#include <stdlib.h>

#include "foreach.hpp"
#include "wml_node.hpp"

#include <boost/lexical_cast.hpp>

#define WML_READ_VECTOR(node, v, create_statement, element, ptr) \
	{ \
		wml::node::const_child_range range = node->get_child_range(element); \
		while(range.first != range.second) { \
			v.push_back(ptr(create_statement(range.first->second))); \
			++range.first; \
		} \
	}

#define WML_WRITE_ATTR(node, var) \
	{ \
		node->set_attr(#var, boost::lexical_cast<std::string>(var##_)); \
	}

namespace wml {

struct error {};

typedef std::vector<const_node_ptr> const_node_vector;
typedef std::vector<node_ptr> node_vector;

node_ptr deep_copy(const_node_ptr node);
node_ptr deep_copy(const_node_ptr node, const std::string& name);
void merge_attr_over(const_node_ptr src, node_ptr dst);
void merge_over(const_node_ptr src, node_ptr dst);
void copy_over(const_node_ptr src, node_ptr dst);

//find a child which has a given attribute matching
node_ptr find_child_by_attribute(node_ptr node, const std::string& element_name, const std::string& attr, const std::string& value);

std::vector<const_node_ptr> child_nodes(const const_node_ptr& ptr,
                                        const std::string& element);
std::vector<node_ptr> child_nodes(const node_ptr& ptr,
                                  const std::string& element);

inline const std::string& get_str(const_node_ptr ptr,
                                  const std::string& key)
{
	return (*ptr)[key];
}

inline const std::string& get_str(const_node_ptr ptr,
                           const std::string& key,
						   const std::string& default_val)
{
	const std::string& res = (*ptr)[key];
	if(res.empty()) {
		return default_val;
	}

	return res;
}

inline bool get_bool(const_node_ptr ptr, const std::string& key,
                     bool default_val=false)
{
	const std::string& str = get_str(ptr,key);
	if(str.empty()) {
		return default_val;
	}
	return str == "yes" || str == "true";
}

template<typename T>
T get_attr(const_node_ptr ptr, const std::string& key,
           T default_value=T())
{
	if(!ptr) {
		return default_value;
	}

	try {
		return boost::lexical_cast<T>((*ptr)[key]);
	} catch(boost::bad_lexical_cast&) {
		return default_value;
	}
}

inline int get_int(const_node_ptr ptr, const std::string& key,
                   int default_value=0)
{
	if(!ptr) {
		return default_value;
	}

	const std::string& str = (*ptr)[key];
	if(str.empty()) {
		return default_value;
	}

	return atoi(str.c_str());
}

inline bool require(bool cond)
{
	if(!cond) {
		throw error();
	}
}

std::vector<int> get_vector_int(const_node_ptr ptr, const std::string& key);

template<typename Value>
node_ptr write_attribute_map(const std::string& name, const std::map<std::string,Value>& vals)
{
	node_ptr res(new node(name));
	for(typename std::map<std::string,Value>::const_iterator i = vals.begin(); i != vals.end(); ++i) {
		res->set_attr(i->first, boost::lexical_cast<std::string>(i->second));
	}

	return res;
}

class child_sequence_iterator {
public:
	child_sequence_iterator(wml::const_node_ptr node, const std::string& child_name);
	void next();
	bool at_end() const;
	operator wml::const_node_ptr() const {
		return i1_->second;
	}

	wml::const_node_ptr operator->() const {
		return i1_->second;
	}

	const wml::node& operator*() const {
		return *get();
	}

	wml::const_node_ptr get() const {
		return i1_->second;
	}
private:
	wml::node::const_child_iterator i1_, i2_;
};

}

#define FOREACH_WML_CHILD(item, node, element_name) \
  for(wml::child_sequence_iterator item(node, element_name); !item.at_end(); item.next())

#endif
