
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef WML_PARSER_HPP_INCLUDED
#define WML_PARSER_HPP_INCLUDED

#include <string>

#include "wml_node.hpp"

namespace wml
{

class schema;

struct parse_error {
	parse_error(const std::string& msg);
	std::string message;
};

node_ptr parse_wml(const std::string& doc, bool must_have_doc=true, const schema* schema=NULL);

node_ptr parse_wml_from_file(const std::string& fname, const schema* schema=NULL, bool must_have_doc=true);

}

#endif
