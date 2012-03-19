
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <cassert>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include <string.h>

#include <boost/shared_ptr.hpp>

#include "asserts.hpp"
#include "concurrent_cache.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "preprocessor.hpp"
#include "string_utils.hpp"
#include "unit_test.hpp"
#include "wml_parser.hpp"
#include "wml_schema.hpp"
#include "wml_utils.hpp"

namespace wml
{

namespace {

//string to provide iterators for errors that don't have a location.
std::string NullErrorLocStr;

class wml_template
{
public:
	explicit wml_template(const node_ptr& node) : node_(node) {
	}

	void add_argument(const std::string& arg) {
		args_.push_back(arg);
	}

	void set_node(const node_ptr& node) {
		node_ = node;
	}

	node_ptr call(const std::vector<std::string>& args) const;
private:
	node_ptr node_;
	std::vector<std::string> args_;
};

typedef boost::shared_ptr<wml_template> wml_template_ptr;
typedef concurrent_cache<std::string,wml_template_ptr> wml_template_map;
wml_template_map wml_templates;

bool substitute_single_attr(const std::string& arg_name,
							const std::string& arg_value,
							std::string* str)
{
	if(const char* pos = strstr(str->c_str(),arg_name.c_str())) {
		str->replace(pos-str->c_str(), arg_name.size(), arg_value);
		return true;
	}

	return false;
}

bool substitute_attr(const std::string& attr,
                     const std::vector<std::string>& arg_names,
                     const std::vector<std::string>& arg_values,
					 std::string* result)
{
	assert(arg_names.size() == arg_values.size());
	*result = attr;
	bool diff = false;
	for(unsigned int n = 0; n != arg_names.size(); ++n) {
		while(substitute_single_attr(arg_names[n],arg_values[n],result)) {
			diff = true;
		}
	}

	return diff;
}

node_ptr substitute(node_ptr node,
                    const std::vector<std::string>& arg_names,
                    const std::vector<std::string>& arg_values)
{
	assert(arg_names.size() == arg_values.size());
	wml::node::const_all_child_iterator i1 = node->begin_children();
	wml::node::const_all_child_iterator i2 = node->end_children();
	std::vector<node_ptr> children;
	bool is_different = false;
	while(i1 != i2) {
		node_ptr child = *i1;
		node_ptr child_copy = substitute(child,arg_names,arg_values);
		children.push_back(child_copy);
		if(child != child_copy) {
			is_different = true;
		}
		++i1;
	}

	std::map<std::string,std::string> attr;
	wml::node::const_attr_iterator a1 = node->begin_attr();
	wml::node::const_attr_iterator a2 = node->end_attr();
	while(a1 != a2) {
		std::string new_str;
		if(substitute_attr(a1->second,arg_names,arg_values,&new_str)) {
			attr[a1->first] = new_str;
		}
		++a1;
	}

	if(is_different || attr.empty() == false) {
		node_ptr res(new wml::node(node->name()));
		a1 = node->begin_attr();
		while(a1 != a2) {
			std::map<std::string,std::string>::const_iterator i = attr.find(a1->first);
			if(i != attr.end()) {
				res->set_attr(i->first, i->second);
			} else {
				res->set_attr(a1->first, a1->second);
			}
			++a1;
		}

		foreach(const wml::node_ptr& child, children) {
			res->add_child(child);
		}

		return res;
	} else {
		return node;
	}
}

node_ptr wml_template::call(const std::vector<std::string>& args) const
{
	if(args.size() != args_.size()) {
		throw parse_error(formatter() << "incorrect number of arguments when calling '" << node_->name() << "'\n");
	}

	return substitute(node_, args_, args);
}

std::string parse_element(std::string::const_iterator& i1,
                          std::string::const_iterator i2)
{
	const std::string::const_iterator start_pos = i1;

	++i1;
	const std::string::const_iterator beg = i1;
	while(*i1 != ']' && i1 != i2) {
		++i1;
	}

	if(i1 == i2) {
		throw parse_error("unexpected end of wml document while parsing element", start_pos);
	}

	std::string res(beg,i1);
	util::strip(res);
	++i1;

	if(res.empty()) {
		throw parse_error("empty element", start_pos);
	}

	return res;
}

std::string parse_name(std::string::const_iterator& i1,
                       std::string::const_iterator i2)
{
	const std::string::const_iterator beg = i1;
	i1 = std::find(i1,i2,'=');
	if(i1 == i2) {
		throw parse_error("unexpected end of wml document while parsing name", beg);
	}

	std::string res(beg,i1);
	util::strip(res);

#if !TARGET_OS_IPHONE
	for(std::string::const_iterator i = res.begin(); i != res.end(); ++i) {
		if(!util::isalnum(*i) && *i != '_') {
			throw parse_error("illegal character in wml attribute name", beg + (i - res.begin()));
		}
	}
#endif

	return res;
}

std::string parse_value(std::string::const_iterator& i1,
                        std::string::const_iterator i2,
						int& line_number, int& line_starts_at)
{
	line_starts_at = line_number;
	std::string res;
	const std::string::const_iterator beg = i1;
	while(i1 != i2 && !util::isnewline(*i1) && *i1 != '#') {
		if(*i1 == '"') {
			++i1;
			while(i1 != i2 && *i1 != '"') {
				if(*i1 == '\\' && i1+1 != i2) {
					++i1;
					// if "\\n" is encountered, push it
					// through verbatim.
					if (*i1 == 'n') {
						res.push_back('\\');
						continue;
					}
				}
				if(util::isnewline(*i1)) {
					++line_number;
				}
				res.push_back(*i1);
				++i1;
			}

			if(i1 == i2) {
				break;
			}

			++i1;
			continue;
		}

		res.push_back(*i1);

		++i1;
	}

	if(i1 == i2) {
		throw parse_error(formatter() << "unexpected end of wml document while parsing value '" << std::string(beg,i1) << "'", beg);
	}

	//strip the string, but leave it untouched if it's all whitespace.
	{
		int newlines = 0;
		std::string::iterator i = res.begin();
		while(i != res.end() && util::isspace(*i)) {
			if(*i == '\n') {
				++newlines;
			}
			++i;
		}

		if(i != res.end()) {
			res.erase(res.begin(), i);
			while(util::isspace(res[res.size()-1])) {
				res.resize(res.size()-1);
			}

			line_starts_at += newlines;
		}
	}

	return res;
}

void skip_comment(std::string::const_iterator& i1,
                  std::string::const_iterator i2)
{
	while(i1 != i2 && !util::isnewline(*i1)) {
		++i1;
	}
}

int get_line_number(const std::string& doc, std::string::const_iterator i)
{
	return std::count(doc.begin(), i, '\n') + 1;
}

namespace {
concurrent_cache<std::string, std::string*> filename_pool;

struct node_frame {
	node_frame() : derived_frame(false) {}

	node_ptr node;

	//all 'bases' defined for nodes that we parse in this element.
	std::map<std::string, node_ptr> base_nodes;

	//if we are derived, then we don't do checks for multiply defined
	//attributes.
	bool derived_frame;
};

}

node_ptr parse_wml_internal(const std::string& error_context, const std::string& doc, bool must_have_doc, const schema* current_schema)
{
#define PARSE_ERROR(msg, loc) throw parse_error(formatter() << error_context << " line " << line_number << ": " << msg, loc);
	node_ptr res;
	std::stack<node_frame> nodes;
	std::stack<const schema*> schemas;
	schemas.push(NULL);
	std::string::const_iterator i = doc.begin();
	std::string current_comment;
	int line_number = 1;

	const std::string* filename_ptr = filename_pool.get(error_context);
	if(!filename_ptr) {
		std::string* str = new std::string(error_context);
		filename_ptr = str;
		filename_pool.put(error_context, str);
	}

	try {

	while(i != doc.end()) {
		if(util::isnewline(*i)) {
			++i;
			++line_number;
		} else if(util::isspace(*i)) {
			++i;
		} else if(*i == '[') {

			const std::string::const_iterator begin_element = i;
			std::string element = parse_element(i,doc.end());
			if(element[0] == '/') {
				const std::string close_element(element.begin()+1,element.end());
				if(nodes.empty()) {
					PARSE_ERROR("close element found when there are no elements", begin_element);
				} else if(nodes.top().node->name() != close_element) {
					PARSE_ERROR("mismatch between open and close elements: '" << nodes.top().node->name() << "' vs '" << close_element << "'", begin_element);
				}

				if(schemas.top()) {
					schemas.top()->validate_node(nodes.top().node);
				}

				schemas.pop();
				nodes.pop();
			} else {
				if(nodes.empty() && res) {
					PARSE_ERROR("multiple root elements found", begin_element);
				}

				if(nodes.empty()) {
					schemas.push(current_schema);
				} else {
					const schema* new_schema = NULL;
					if(schemas.top()) {
						new_schema = schemas.top()->validate_element(element);
					}

					schemas.push(new_schema);
				}

				std::string prefix;
				std::string::iterator colon =
				   std::find(element.begin(),element.end(),':');
				if(colon != element.end()) {
					prefix.assign(element.begin(),colon);
					element.erase(element.begin(),colon+1);
				}

				std::vector<wml::node_ptr> parents;

				std::string::iterator begin_args =
				   std::find(element.begin(),element.end(),'(');
				if(prefix != "template" && begin_args != element.end()) {
					std::string::iterator end_args =
					  std::find(begin_args+1,element.end(),')');
					std::string args_str(begin_args+1,end_args);
					std::vector<std::string> args =
					     util::split(args_str);

					element.erase(begin_args,element.end());

					const wml_template_ptr t = wml_templates.get(element);
					if(t.get() == NULL) {
						PARSE_ERROR("unrecognized template call: '" << element << "'\n", i);
					}

					const wml::node_ptr el = t->call(args);
					assert(el);
					if(!prefix.empty()) {
						parents.push_back(el);
						element = prefix;
						prefix.clear();
					} else if(nodes.empty()) {
						res = el;
						continue;
					} else {
						nodes.top().node->add_child(el);
						continue;
					}
				}

				bool derived_node = false;
				node_ptr el(new node(element));
				if(!nodes.empty()) {
					//see if we have a base node to use recorded to base this
					//node off, rather than starting fresh.
					std::map<std::string, node_ptr>::const_iterator itor = nodes.top().base_nodes.find(element);
					if(itor != nodes.top().base_nodes.end()) {
						el = deep_copy(itor->second);
						derived_node = true;
					}
				}

				el->set_schema(schemas.top());
				foreach(const wml::node_ptr& parent, parents) {
					wml::merge_over(parent, el);
				}
				if(current_comment.empty() == false) {
					el->set_comment(current_comment);
					current_comment.clear();
				}

				if(prefix == "template") {
					std::string::iterator template_name =
					   std::find(element.begin(),element.end(),' ');
					if(template_name == element.end()) {
						PARSE_ERROR("no template name found for template '" << element << "'", i);
					}

					el.reset(new node(std::string(element.begin(),template_name)));

					while(template_name != element.end() && util::isspace(*template_name)) {
						++template_name;
					}

					std::string::iterator beg_args =
					   std::find(template_name,element.end(),'(');
					std::string::iterator end_args =
					   std::find(template_name,element.end(),')');
					if(beg_args == element.end() || end_args == element.end()) {
						PARSE_ERROR("no arg list found for template '" << element << "'", i);
					}

					wml_template_ptr t(new wml_template(el));

					std::string args_str(beg_args+1,end_args);
					std::vector<std::string> args;
					if(!args_str.empty()) {
						args = util::split(args_str);
						foreach(const std::string& arg, args) {
							t->add_argument("{" + arg + "}");
						}
					}

					std::string template_name_str(template_name,beg_args);
					util::strip(template_name_str);
					wml_templates.put(template_name_str, t);
				} else if(prefix == "base") {
					el->set_prefix("base");
					nodes.top().base_nodes[element] = el;
					nodes.top().node->set_base_element(element, el);
					//std::cerr << "SET BASE: " << nodes.top().node->name() << " -> " << element << "\n";
				} else if(prefix != "") {
					PARSE_ERROR("unrecognized element prefix", i);
				} else if(nodes.empty()) {
					res = el;
				} else {
					nodes.top().node->add_child(el);
				}

				node_frame frame;
				frame.node = el;
				frame.derived_frame = derived_node;
				nodes.push(frame);
			}
		} else if(util::isalnum(*i) || *i == '_') {
			if(nodes.empty()) {
				PARSE_ERROR("attributes found at root", i);
			}

			std::string::const_iterator name_start = i;

			const std::string name = parse_name(i,doc.end());
			int attr_line = line_number;
			++i;
			const std::string value = parse_value(i,doc.end(), line_number, attr_line);

			if(schemas.top()) {
				schemas.top()->validate_attribute(name, value);
			}

			if(!nodes.top().derived_frame && nodes.top().node->has_attr(name)) {
				PARSE_ERROR("attribute appears multiple times", name_start);
			}
			nodes.top().node->set_attr(name, wml::value(value, filename_ptr, attr_line));
			nodes.top().node->add_attr_order(name);

			if(current_comment.empty() == false) {
				nodes.top().node->set_attr_comment(name, current_comment);
				current_comment.clear();
			}
		} else if(*i == '@') {
			++i;
			std::string::const_iterator begin = i;
			while(i != doc.end() && !util::isspace(*i) && !util::isnewline(*i)) {
				++i;
			}

			std::string name(begin,i);
			if(name == "import" || name == "include") {
				if(i == doc.end()) {
					PARSE_ERROR("unexpected document end while importing", i);
				}

				++i;
				begin = i;
				while(i != doc.end() && !util::isspace(*i) && !util::isnewline(*i)) {
					++i;
				}

				const std::string filename(begin,i);
				if(nodes.empty() && name == "import") {
					PARSE_ERROR("@import statement at top level", i);
				}
				wml::node_ptr node(parse_wml_from_file("data/" + filename, NULL, name == "import"));
				if(node) {
					nodes.top().node->add_child(node);
				}

			} else {
				PARSE_ERROR("unrecognized @ instruction: '" << name << "'", i);
			}
		} else if(*i == '#') {
			std::string::const_iterator begin_comment = i;
			skip_comment(i,doc.end());
			current_comment.insert(current_comment.end(), begin_comment, i);
		} else {
			std::cerr << "unexpected chars: {{{" << &*i << "}}}\n";
			PARSE_ERROR("unexpected characters in wml document", i);
		}
	} // while(i != doc.end())
	} catch(const schema_error& e) {
		PARSE_ERROR(e.message, NullErrorLocStr.end());
	}

	if(must_have_doc && !res) {
		PARSE_ERROR("empty wml document for filename '" << doc <<"'", NullErrorLocStr.end());
	}

	if(nodes.empty() == false) {
		PARSE_ERROR("unexpected end of wml document", NullErrorLocStr.end());
	}

	return res;
#undef PARSE_ERROR
}

}  // namespace

node_ptr parse_wml(const std::string& doc, bool must_have_doc, const schema* schema)
{
	return parse_wml_internal("", doc, must_have_doc, schema);
}

node_ptr parse_wml_from_file(const std::string& fname, const schema* schema, bool must_have_doc)
{
    
	const std::string data = preprocess(sys::read_file(fname));
	if(data.empty()) {
		ASSERT_LOG(false, "Could not read file: " << fname);
	}
	try{
		return parse_wml_internal(fname, data, must_have_doc, schema);
	} catch(wml::parse_error& e) {
		if(e.error_loc >= data.begin() && e.error_loc < data.end()) {
			std::string::const_iterator i1 = e.error_loc;
			while(i1 != data.begin() && *i1 != '\n') {
				--i1;
			}

			while(i1 != e.error_loc && util::isspace(*i1)) {
				++i1;
			}

			std::string::const_iterator i2 = std::find(e.error_loc, data.end(), '\n');
			int spaces = e.error_loc - i1;
			const int line_num = std::count(data.begin(), i1, '\n') + 1;
			std::cerr << "ERROR PARSING WML IN " << fname << " on line " << line_num << ":\n";
			std::cerr << std::string(i1, i2) << "\n";
			while(spaces--) {
				std::cerr << " ";
			}

			std::cerr << "^\n";
			ASSERT_LOG(false, e.message);
		} else {

			ASSERT_LOG(false, "Error parsing WML in " << fname << ": " << e.message);
		}
	} catch(...) {
		ASSERT_LOG(false, "Unknown error loading WML in " << fname);
	}
}

parse_error::parse_error(const std::string& msg)
  : message(msg), error_loc(NullErrorLocStr.end())
{}

parse_error::parse_error(const std::string& msg, std::string::const_iterator i)
  : message(msg), error_loc(i)
{}

}

UNIT_TEST(wml_parser_test) {
	const std::string doc =
"[doc]\n"
"attr=blah\n"
"  [base:test]\n"
"  x=y\n"
"  [/test]\n"
"  [test]\n"
"  a=b\n"
"  [/test]\n"
"[/doc]\n";

	wml::const_node_ptr node(wml::parse_wml(doc));
	CHECK_EQ(node->name(), "doc");
	CHECK_EQ(node->attr("attr").str(), "blah");

	wml::const_node_ptr test_node = node->get_child("test");
	CHECK_NE(test_node, wml::const_node_ptr());

	CHECK_EQ(test_node->attr("x").str(), "y");
	CHECK_EQ(test_node->attr("a").str(), "b");
}
