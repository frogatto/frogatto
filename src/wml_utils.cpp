#include "wml_utils.hpp"

namespace wml {

node_ptr deep_copy(const_node_ptr ptr, const std::string& name)
{
	node_ptr res(new node(name));
	res->set_comment(ptr->get_comment());
	for(node::const_attr_iterator i = ptr->begin_attr();
	    i != ptr->end_attr(); ++i) {
		res->set_attr(i->first,i->second);
		const std::string& comment = ptr->get_attr_comment(i->first);
		if(comment.empty() == false) {
			res->set_attr_comment(i->first,comment);
		}
	}

	for(node::const_all_child_iterator i = ptr->begin_children();
	    i != ptr->end_children(); ++i) {
		res->add_child(deep_copy(*i));
	}

	return res;
}

node_ptr deep_copy(const_node_ptr ptr)
{
	return deep_copy(ptr, ptr->name());
}

void merge_over(const_node_ptr src, node_ptr dst)
{
	if(src->get_comment().empty() == false) {
		dst->set_comment(src->get_comment());
	}

	for(node::const_attr_iterator i = src->begin_attr();
	    i != src->end_attr(); ++i) {
		dst->set_attr(i->first,i->second);
		const std::string& comment = src->get_attr_comment(i->first);
		if(comment.empty() == false) {
			dst->set_attr_comment(i->first,comment);
		}
	}

	for(node::const_all_child_iterator i = src->begin_children();
	    i != src->end_children(); ++i) {
		dst->add_child(deep_copy(*i));
	}
}

void copy_over(const_node_ptr src, node_ptr dst)
{
	dst->clear_attr();
	dst->clear_children();
	merge_over(src, dst);
}

std::vector<const_node_ptr> child_nodes(const const_node_ptr& ptr,
                                        const std::string& element)
{
	std::vector<const_node_ptr> res;
	if(!ptr) {
		return res;
	}

	wml::node::const_child_range range = ptr->get_child_range(element);
	while(range.first != range.second) {
		res.push_back(range.first->second);
		++range.first;
	}

	return res;
}

std::vector<node_ptr> child_nodes(const node_ptr& ptr,
                                  const std::string& element)
{
	std::vector<node_ptr> res;
	if(!ptr) {
		return res;
	}

	wml::node::child_range range = ptr->get_child_range(element);
	while(range.first != range.second) {
		res.push_back(range.first->second);
		++range.first;
	}

	return res;
}

}
