
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <algorithm>

#include "wml_node.hpp"

namespace wml
{

namespace {
	const value empty_value("");
}

const value& node::operator[](const std::string& key) const
{
		attr_map::const_iterator itor = attr_.find(key);
		if(itor == attr_.end()) {
			return empty_value;
		}

		return itor->second;
}

const value& node::attr(const std::string& key) const
{
	return (*this)[key];
}

void node::set_attr(const std::string& key, const value& val)
{
	attr_[key] = val;
}

void node::set_or_erase_attr(const std::string& key, const std::string& value)
{
	if(value.empty() == false) {
		set_attr(key,value);
	} else {
		attr_.erase(key);
	}
}

void node::erase_attr(const std::string& key)
{
	attr_.erase(key);
	if(!attr_order_.empty()) {
		std::vector<std::string>::iterator itor = std::find(attr_order_.begin(), attr_order_.end(), key);
		if(itor != attr_order_.end()) {
			attr_order_.erase(itor);
		}
	}
}

bool node::has_attr(const std::string& key) const
{
	attr_map::const_iterator itor = attr_.find(key);
	return itor != attr_.end() && itor->second.empty() == false;
}

node::const_attr_iterator node::begin_attr() const
{
	return attr_.begin();
}

node::const_attr_iterator node::end_attr() const
{
	return attr_.end();
}

node::child_iterator node::begin_child(const std::string& key)
{
	return childmap_.lower_bound(key);
}

node::const_child_iterator node::begin_child(const std::string& key) const
{
	return childmap_.lower_bound(key);
}

node::child_iterator node::end_child(const std::string& key)
{
	return childmap_.upper_bound(key);
}

node::const_child_iterator node::end_child(const std::string& key) const
{
	return childmap_.upper_bound(key);
}

node::child_range node::get_child_range(const std::string& key)
{
	return childmap_.equal_range(key);
}

node::const_child_range
node::get_child_range(const std::string& key) const
{
	return childmap_.equal_range(key);
}

node::all_child_iterator node::begin_children()
{
	return children_.begin();
}

node::const_all_child_iterator node::begin_children() const
{
	return children_.begin();
}

node::all_child_iterator node::end_children()
{
	return children_.end();
}

node::const_all_child_iterator node::end_children() const
{
	return children_.end();
}

void node::add_child(boost::shared_ptr<node> child)
{
	childmap_.insert(std::pair<std::string,boost::shared_ptr<node> >(
							child->name(),child));
	children_.push_back(child);
}

const_node_ptr node::get_child(const std::string& key) const
{
	const const_child_range range = get_child_range(key);
	if(range.first == range.second) {
		return const_node_ptr();
	} else {
		return range.first->second;
	}
}
node_ptr node::get_child(const std::string& key)
{
	const child_range range = get_child_range(key);
	if(range.first == range.second) {
		return node_ptr();
	} else {
		return range.first->second;
	}
}

void node::clear_attr()
{
	attr_.clear();
}

void node::clear_children()
{
	childmap_.clear();
	children_.clear();
}

namespace {
class node_name_equals {
	std::string name_;
public:
	node_name_equals(const std::string& name) : name_(name)
	{}

	bool operator()(const wml::const_node_ptr& node) const {
		return node->name() == name_;
	}
};
}

void node::clear_children(const std::string& name)
{
	childmap_.erase(name);
	children_.erase(std::remove_if(children_.begin(),children_.end(),node_name_equals(name)), children_.end());
}

void node::erase_child(const boost::shared_ptr<node>& child_node)
{
	child_range range = get_child_range(child_node->name());
	while(range.first != range.second) {
		if(range.first->second == child_node) {
			childmap_.erase(range.first);
			break;
		}
		++range.first;
	}

	std::vector<boost::shared_ptr<node> >::iterator i = std::find(children_.begin(),children_.end(),child_node);
	if(i != children_.end()) {
		children_.erase(i);
	}
}

void node::set_comment(const std::string& comment)
{
	comment_ = comment;
}

const std::string& node::get_comment() const
{
	return comment_;
}

void node::set_attr_comment(const std::string& name, const std::string& comment)
{
	attr_comments_[name] = comment;
}

const std::string& node::get_attr_comment(const std::string& name) const
{
	attr_map::const_iterator i = attr_comments_.find(name);
	if(i != attr_comments_.end()) {
		return i->second;
	} else {
		return empty_value;
	}
}

void node::add_attr_order(const std::string& attr)
{
	attr_order_.push_back(attr);
}

void node::set_base_element(const std::string& key, wml::const_node_ptr node)
{
	base_elements_[key] = node;
}

wml::const_node_ptr node::get_base_element(const std::string& key) const
{
	std::map<std::string, wml::const_node_ptr>::const_iterator itor = base_elements_.find(key);
	if(itor != base_elements_.end()) {
		return itor->second;
	} else {
		return wml::const_node_ptr();
	}
}

void node::strip_prettiness()
{
	comment_.clear();
	attr_comments_.clear();
	attr_order_.clear();
	base_elements_.clear();

	for(std::vector<boost::shared_ptr<node> >::iterator i = children_.begin();
	    i != children_.end(); ++i) {
		(*i)->strip_prettiness();
	}
}


}
