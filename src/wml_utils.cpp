#include "string_utils.hpp"
#include "unit_test.hpp"
#include "wml_utils.hpp"

namespace wml {

node_ptr deep_copy(const_node_ptr ptr, const std::string& name)
{
	node_ptr res(new node(name));
	res->set_schema(ptr->get_schema());
	res->set_comment(ptr->get_comment());
	for(std::map<std::string, wml::const_node_ptr>::const_iterator i = ptr->base_elements().begin(); i != ptr->base_elements().end(); ++i) {
		std::cerr << "COPY: " << ptr->name() << " -> " << i->first << "\n";
		res->set_base_element(i->first, i->second);
	}

	for(std::vector<std::string>::const_iterator i = ptr->attr_order().begin();
	    i != ptr->attr_order().end(); ++i) {
		res->add_attr_order(*i);
	}

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

void merge_attr_over(const_node_ptr src, node_ptr dst)
{
	for(node::const_attr_iterator i = src->begin_attr();
	    i != src->end_attr(); ++i) {
		dst->set_attr(i->first,i->second);
		const std::string& comment = src->get_attr_comment(i->first);
		if(comment.empty() == false) {
			dst->set_attr_comment(i->first,comment);
		}
	}
}

void merge_over(const_node_ptr src, node_ptr dst)
{
	if(src->get_comment().empty() == false) {
		dst->set_comment(src->get_comment());
	}

	merge_attr_over(src, dst);

	for(std::map<std::string, wml::const_node_ptr>::const_iterator i = src->base_elements().begin(); i != src->base_elements().end(); ++i) {
		dst->set_base_element(i->first, i->second);
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

node_ptr find_child_by_attribute(node_ptr node, const std::string& element_name, const std::string& attr, const std::string& value)
{
	node::child_iterator i1 = node->begin_child(element_name);
	node::child_iterator i2 = node->end_child(element_name);
	while(i1 != i2) {
		if(i1->second->attr(attr).str() == value) {
			return i1->second;
		}

		++i1;
	}

	return node_ptr();
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

child_sequence_iterator::child_sequence_iterator(wml::const_node_ptr node, const std::string& child_name)
  : i1_(node->begin_child(child_name)), i2_(node->end_child(child_name))
{}

void child_sequence_iterator::next()
{
	++i1_;
}

bool child_sequence_iterator::at_end() const
{
	return i1_ == i2_;
}

std::vector<int> get_vector_int(const_node_ptr ptr, const std::string& key)
{
	std::vector<std::string> v = util::split(ptr->attr(key));
	std::vector<int> result(v.size());
	for(int n = 0; n != v.size(); ++n) {
		result[n] = atoi(v[n].c_str());
	}

	return result;
}

}

BENCHMARK(wml_get_int)
{
	wml::node_ptr node(new wml::node("a"));
	node->set_attr("abc", "47");
	BENCHMARK_LOOP {
	}
}
