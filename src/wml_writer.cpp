#include <iostream>
#include <set>

#include "foreach.hpp"
#include "string_utils.hpp"
#include "unit_test.hpp"
#include "wml_node.hpp"
#include "wml_writer.hpp"

namespace wml
{

namespace {
void write_comment(const std::string& comment, const std::string& indent, std::string& res)
{
	std::vector<std::string> lines = util::split(comment, '\n');
	foreach(const std::string& line, lines) {
		res += indent + line + "\n";
	}
}
}

void write(const wml::const_node_ptr& node, std::string& res)
{
	std::string indent;
	write(node,res,indent);
}

namespace {
std::string escape_quotes(const std::string& s) {
	std::string res;
	foreach(char c, s) {
		if(c == '"') {
			res.push_back('\\');
		}

		res.push_back(c);
	}

	return res;
}
}

void write(const wml::const_node_ptr& node, std::string& res,
           std::string& indent, wml::const_node_ptr base)
{
	if(node->get_comment().empty() == false) {
		write_comment(node->get_comment(), indent, res);
	}
	res += indent + "[";
	if(node->prefix().empty() == false) {
		res += node->prefix() + ":";
	}
	
	res += node->name() + "]\n";

	const std::vector<std::string>& attr_order = node->attr_order();
	foreach(const std::string& attr, attr_order) {
		if(base && base->attr(attr).str() == node->attr(attr).str()) {
			continue;
		}

		const std::string& comment = node->get_attr_comment(attr);
		if(comment.empty() == false) {
			write_comment(comment, indent, res);
		}
		res += indent + attr + "=\"" + escape_quotes(node->attr(attr).str()) + "\"\n";
	}

	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		if(std::find(attr_order.begin(), attr_order.end(), i->first) != attr_order.end()) {
			continue;
		}

		if(base && base->attr(i->first).str() == i->second.str()) {
			continue;
		}

		const std::string& comment = node->get_attr_comment(i->first);
		if(comment.empty() == false) {
			write_comment(comment, indent, res);
		}
		res += indent + i->first + "=\"" + escape_quotes(i->second.str()) + "\"\n";
	}
	indent.push_back('\t');

	std::set<std::string> base_written;
	for(wml::node::const_all_child_iterator i = node->begin_children();
	    i != node->end_children(); ++i) {
		wml::const_node_ptr base_node = node->get_base_element((*i)->name());
		if(base_node && base_written.count((*i)->name()) == 0) {
			base_written.insert((*i)->name());
			if(base_node) {
				write(base_node, res, indent);
			}
		}

		write(*i, res, indent, base_node);
	}
	indent.resize(indent.size()-1);
	res += indent + "[/" + node->name() + "]\n\n";
}

std::string output(const wml::const_node_ptr& node)
{
	std::string res;
	write(node, res);
	return res;
}

}
