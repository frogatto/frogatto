
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <cctype>
#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <vector>

#include <string.h>

#include <boost/shared_ptr.hpp>

#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "preprocessor.hpp"
#include "string_utils.hpp"
#include "wml_parser.hpp"
#include "wml_schema.hpp"
#include "wml_utils.hpp"

namespace wml
{

namespace {

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
typedef std::map<std::string,wml_template_ptr> wml_template_map;
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
	++i1;
	const std::string::const_iterator beg = i1;
	while(*i1 != ']' && i1 != i2) {
		++i1;
	}

	if(i1 == i2) {
		throw parse_error("unexpected end of wml document while parsing element");
	}

	std::string res(beg,i1);
	util::strip(res);
	++i1;

	if(res.empty()) {
		throw parse_error("empty element");
	}

	return res;
}

std::string parse_name(std::string::const_iterator& i1,
                       std::string::const_iterator i2)
{
	const std::string::const_iterator beg = i1;
	i1 = std::find(i1,i2,'=');
	if(i1 == i2) {
		throw parse_error("unexpected end of wml document while parsing name");
	}

	std::string res(beg,i1);
	util::strip(res);
	return res;
}

std::string parse_value(std::string::const_iterator& i1,
                        std::string::const_iterator i2)
{
	std::string res;
	const std::string::const_iterator beg = i1;
	while(i1 != i2 && !util::isnewline(*i1) && *i1 != '#') {
		if(*i1 == '"') {
			++i1;
			while(i1 != i2 && *i1 != '"') {
				if(*i1 == '\\' && i1+1 != i2) {
					++i1;
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
		throw parse_error(formatter() << "unexpected end of wml document while parsing value '" << std::string(beg,i1) << "'");
	}

	util::strip(res);
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

node_ptr parse_wml_internal(const std::string& error_context, const std::string& doc, bool must_have_doc, const schema* current_schema)
{
#define PARSE_ERROR(msg) throw parse_error(formatter() << error_context << " line " << get_line_number(doc, i) << ": " << msg);
	node_ptr res;
	std::stack<node_ptr> nodes;
	std::stack<const schema*> schemas;
	schemas.push(NULL);
	std::string::const_iterator i = doc.begin();
	std::string current_comment;

	try {
	while(i != doc.end()) {
		if(isspace(*i) || util::isnewline(*i)) {
			++i;
		} else if(*i == '[') {
			std::string element = parse_element(i,doc.end());
			if(element[0] == '/') {
				const std::string close_element(element.begin()+1,element.end());
				if(nodes.empty()) {
					PARSE_ERROR("close element found when there are no elements");
				} else if(nodes.top()->name() != close_element) {
					PARSE_ERROR("mismatch between open and close elements: '" << nodes.top()->name() << "' vs '" << close_element << "'");
				}

				if(schemas.top()) {
					schemas.top()->validate_node(nodes.top());
				}

				schemas.pop();
				nodes.pop();
			} else {
				if(nodes.empty() && res) {
					PARSE_ERROR("multiple root elements found");
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

					const wml_template_map::const_iterator t =
					           wml_templates.find(element);
					if(t == wml_templates.end()) {
						PARSE_ERROR("unrecognized template call: '" << element << "'\n");
					}

					const wml::node_ptr el = t->second->call(args);
					assert(el);
					if(!prefix.empty()) {
						parents.push_back(el);
						element = prefix;
						prefix.clear();
					} else if(nodes.empty()) {
						res = el;
						continue;
					} else {
						nodes.top()->add_child(el);
						continue;
					}
				}

				node_ptr el(new node(element));
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
						PARSE_ERROR("no template name found for template '" << element << "'");
					}

					el.reset(new node(std::string(element.begin(),template_name)));

					while(template_name != element.end() && isspace(*template_name)) {
						++template_name;
					}

					std::string::iterator beg_args =
					   std::find(template_name,element.end(),'(');
					std::string::iterator end_args =
					   std::find(template_name,element.end(),')');
					if(beg_args == element.end() || end_args == element.end()) {
						PARSE_ERROR("no arg list found for template '" << element << "'");
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
					wml_templates[template_name_str] = t;
				} else if(prefix != "") {
					PARSE_ERROR("unrecognized element prefix");
				} else if(nodes.empty()) {
					res = el;
				} else {
					nodes.top()->add_child(el);
				}

				nodes.push(el);
			}
		} else if(isalnum(*i) || *i == '_') {
			if(nodes.empty()) {
				PARSE_ERROR("attributes found at root");
			}

			const std::string name = parse_name(i,doc.end());
			++i;
			const std::string value = parse_value(i,doc.end());

			if(schemas.top()) {
				schemas.top()->validate_attribute(name, value);
			}
			nodes.top()->set_attr(name, value);

			if(current_comment.empty() == false) {
				nodes.top()->set_attr_comment(name, current_comment);
				current_comment.clear();
			}
		} else if(*i == '@') {
			++i;
			std::string::const_iterator begin = i;
			while(i != doc.end() && !isspace(*i) && !util::isnewline(*i)) {
				++i;
			}

			std::string name(begin,i);
			if(name == "import" || name == "include") {
				if(i == doc.end()) {
					PARSE_ERROR("unexpected document end while importing");
				}

				++i;
				begin = i;
				while(i != doc.end() && !isspace(*i) && !util::isnewline(*i)) {
					++i;
				}

				const std::string filename(begin,i);
				if(nodes.empty() && name == "import") {
					PARSE_ERROR("@import statement at top level");
				}
				wml::node_ptr node(parse_wml_from_file("data/" + filename, NULL, name == "import"));
				if(node) {
					nodes.top()->add_child(node);
				}

			} else {
				PARSE_ERROR("unrecognized @ instruction: '" << name << "'");
			}
		} else if(*i == '#') {
			std::string::const_iterator begin_comment = i;
			skip_comment(i,doc.end());
			current_comment.insert(current_comment.end(), begin_comment, i);
		} else {
			std::cerr << "unexpected chars: {{{" << &*i << "}}}\n";
			PARSE_ERROR("unexpected characters in wml document");
		}
	} // while(i != doc.end())
	} catch(const schema_error& e) {
		PARSE_ERROR(e.message);
	}

	if(must_have_doc && !res) {
		PARSE_ERROR("empty wml document");
	}

	if(nodes.empty() == false) {
		PARSE_ERROR("unexpected end of wml document");
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
	return parse_wml_internal(fname, preprocess(sys::read_file(fname)), must_have_doc, schema);
}

parse_error::parse_error(const std::string& msg) : message(msg)
{
	std::cerr << "wml parse error: " << msg << "\n";
}

}

#ifdef WML_PARSER_UNIT_TEST

int main()
{
	const std::string test_sub = "abc {xyz} {abc}";
	std::string res_sub;
	std::vector<std::string> sub_names, sub_values;
	sub_names.push_back("{xyz}");
	sub_names.push_back("{abc}");
	sub_values.push_back("4");
	sub_values.push_back("14");
	wml::substitute_attr(test_sub,sub_names,sub_values,&res_sub);
	assert(res_sub == "abc 4 14");

	const std::string test =
"[template:unit goblin(x,y)]\n"
"x={x}\n"
"y={y}\n"
"[/unit]\n"
"[goblin(10,4)]\n";
	wml::node_ptr node = wml::parse_wml(test);
	assert((*node)["x"] == "10");
	assert((*node)["y"] == "4");
}

#endif
