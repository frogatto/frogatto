#ifndef WML_WRITER_HPP_INCLUDED
#define WML_WRITER_HPP_INCLUDED

#include <string>

#include "wml_node_fwd.hpp"

namespace wml
{
void write(const wml::const_node_ptr& node, std::string& res);
void write(const wml::const_node_ptr& node, std::string& res,
           std::string& indent);
std::string output(const wml::const_node_ptr& node);
}

#endif
