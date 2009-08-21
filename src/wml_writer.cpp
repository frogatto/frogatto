#include "foreach.hpp"
#include "string_utils.hpp"
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

void write(const wml::const_node_ptr& node, std::string& res,
           std::string& indent)
{
	if(node->get_comment().empty() == false) {
		write_comment(node->get_comment(), indent, res);
	}
	res += indent + "[" + node->name() + "]\n";
	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		const std::string& comment = node->get_attr_comment(i->first);
		if(comment.empty() == false) {
			write_comment(comment, indent, res);
		}
		res += indent + i->first + "=\"" + i->second.str() + "\"\n";
	}
	indent.push_back('\t');
	for(wml::node::const_all_child_iterator i = node->begin_children();
	    i != node->end_children(); ++i) {
		write(*i, res, indent);
	}
	indent.resize(indent.size()-1);
	res += indent + "[/" + node->name() + "]\n";
}

std::string output(const wml::const_node_ptr& node)
{
	std::string res;
	write(node, res);
	return res;
}


}
